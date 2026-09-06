/*
 * 调度流程：
 *
 * 初始化：MAG_SchedulerInit()（清空任务池、建立空闲链、启动毫秒时基）
 *     → 列表注册：MAG_SchedulerRegisterTable()（for 循环遍历任务表）
 *     → 单项注册：MAG_SchedulerRegister()（取出空闲节点、保存参数、挂入活动链）
 *     → 循环调度：MAG_SchedulerProcess()（遍历活动链，到期则调用任务函数）
 *     → 执行后处理：更新下次执行时间；次数用完则回收节点
 *     → 返回主循环，继续调用 MAG_SchedulerProcess()。
 *
 * 取消任务：MAG_SchedulerCancel()（根据句柄取消任务；调度期间标记删除，否则立即回收）。
 *
 * 注意：先初始化，再注册；任务在主循环执行，执行间隔从任务函数返回后开始计算。
 */

#include "mag_scheduler.h"
#include "mag_tick.h"
#include <stddef.h>

#if (MAG_SCHEDULER_MAX_TASKS == 0U) || (MAG_SCHEDULER_MAX_TASKS >= 255U)
#error "MAG_SCHEDULER_MAX_TASKS must be between 1 and 254"
#endif

#define MAG_SCHEDULER_NODE_NONE              ((uint8_t)0xFFU)
#define MAG_SCHEDULER_MAX_TIME_DISTANCE_MS   ((uint32_t)0x7FFFFFFFUL)

#define MAG_SCHEDULER_NODE_FREE              0U
#define MAG_SCHEDULER_NODE_ARMED             1U
#define MAG_SCHEDULER_NODE_RUNNING           2U
#define MAG_SCHEDULER_NODE_PENDING_ADD       3U
#define MAG_SCHEDULER_NODE_REMOVE_PENDING    4U

/* 一块任务木牌，next_index 用来把所有活动任务串成单向链表。 */
typedef struct
{
    MAG_TaskCallback_t callback;      // 任务执行函数
    void *p_context;                  // 传给任务函数的参数
    uint32_t next_due_tick;           // 下次执行的 tick 时刻
    uint32_t period_ms;               // 执行间隔，单位 ms
    uint32_t remaining_runs;          // 剩余执行次数
    uint32_t generation;              // 节点代次，用于识别失效句柄
    uint8_t next_index;               // 链表中下一个节点的索引
    uint8_t state;                    // 节点当前状态
} MAG_SchedulerNode_t;

static MAG_SchedulerNode_t s_task_pool[MAG_SCHEDULER_MAX_TASKS];
static uint8_t s_free_head = MAG_SCHEDULER_NODE_NONE;
static uint8_t s_active_head = MAG_SCHEDULER_NODE_NONE;
static uint8_t s_pending_head = MAG_SCHEDULER_NODE_NONE;
static uint8_t s_is_initialized = 0U;
static uint8_t s_is_processing = 0U;

/* 增加节点代次，使已经失效的旧句柄不能操作复用后的任务。 */
static void mag_scheduler_advance_generation(MAG_SchedulerNode_t *p_node)
{
    p_node->generation++;
    if (p_node->generation == 0U)
    {
        p_node->generation = 1U;
    }
}

/* 判断当前 tick 是否已经到达或越过任务规定的执行时刻。 */
static uint8_t mag_scheduler_time_reached(uint32_t now_tick,
                                          uint32_t due_tick)
{
    return ((int32_t)(now_tick - due_tick) >= 0) ? 1U : 0U;
}

/* 检查注册参数是否满足首次延迟、周期和运行次数的约束。 */
static uint8_t mag_scheduler_config_is_valid(const MAG_TaskConfig_t *p_config)
{
    if ((p_config == NULL) || (p_config->callback == NULL))
    {
        return 0U;
    }

    if ((p_config->first_delay_ms > MAG_SCHEDULER_MAX_TIME_DISTANCE_MS) ||
        (p_config->period_ms > MAG_SCHEDULER_MAX_TIME_DISTANCE_MS) ||
        (p_config->run_count == 0U))
    {
        return 0U;
    }

    if ((p_config->run_count != 1U) && (p_config->period_ms == 0U))
    {
        return 0U;
    }

    return 1U;
}

/* 从空闲链表头部取出一块未使用的任务木牌。 */
static uint8_t mag_scheduler_allocate_node(void)
{
    uint8_t node_index;

    if (s_free_head == MAG_SCHEDULER_NODE_NONE)
    {
        return MAG_SCHEDULER_NODE_NONE;
    }

    node_index = s_free_head;
    s_free_head = s_task_pool[node_index].next_index;
    s_task_pool[node_index].next_index = MAG_SCHEDULER_NODE_NONE;

    return node_index;
}

/* 清空一块任务木牌并把它重新挂回空闲链表头部。 */
static void mag_scheduler_release_node(uint8_t node_index)
{
    MAG_SchedulerNode_t *p_node = &s_task_pool[node_index];

    p_node->callback = NULL;
    p_node->p_context = NULL;
    p_node->next_due_tick = 0U;
    p_node->period_ms = 0U;
    p_node->remaining_runs = 0U;
    mag_scheduler_advance_generation(p_node);
    p_node->state = MAG_SCHEDULER_NODE_FREE;
    p_node->next_index = s_free_head;
    s_free_head = node_index;
}

/* 把一块任务木牌直接挂到无序活动链表的头部。 */
static void mag_scheduler_link_active_front(uint8_t node_index)
{
    s_task_pool[node_index].next_index = s_active_head;
    s_active_head = node_index;
}

/* 从活动链表中摘下指定任务木牌。 */
static uint8_t mag_scheduler_unlink_active(uint8_t node_index)
{
    uint8_t previous_index = MAG_SCHEDULER_NODE_NONE;
    uint8_t current_index = s_active_head;

    while (current_index != MAG_SCHEDULER_NODE_NONE)
    {
        if (current_index == node_index)
        {
            if (previous_index == MAG_SCHEDULER_NODE_NONE)
            {
                s_active_head = s_task_pool[current_index].next_index;
            }
            else
            {
                s_task_pool[previous_index].next_index =
                    s_task_pool[current_index].next_index;
            }

            s_task_pool[current_index].next_index = MAG_SCHEDULER_NODE_NONE;
            return 1U;
        }

        previous_index = current_index;
        current_index = s_task_pool[current_index].next_index;
    }

    return 0U;
}

/* 检查任务句柄是否仍然指向同一块有效木牌。 */
static uint8_t mag_scheduler_handle_is_valid(MAG_TaskHandle_t handle)
{
    if (handle.index >= MAG_SCHEDULER_MAX_TASKS)
    {
        return 0U;
    }

    if ((s_task_pool[handle.index].state == MAG_SCHEDULER_NODE_FREE) ||
        (s_task_pool[handle.index].generation != handle.generation))
    {
        return 0U;
    }

    return 1U;
}

/* 清理扫描期间被标记为待删除的活动任务。 */
static void mag_scheduler_cleanup_removed_nodes(void)
{
    uint8_t previous_index = MAG_SCHEDULER_NODE_NONE;
    uint8_t current_index = s_active_head;

    while (current_index != MAG_SCHEDULER_NODE_NONE)
    {
        uint8_t next_index = s_task_pool[current_index].next_index;

        if (s_task_pool[current_index].state ==
            MAG_SCHEDULER_NODE_REMOVE_PENDING)
        {
            if (previous_index == MAG_SCHEDULER_NODE_NONE)
            {
                s_active_head = next_index;
            }
            else
            {
                s_task_pool[previous_index].next_index = next_index;
            }

            mag_scheduler_release_node(current_index);
        }
        else
        {
            previous_index = current_index;
        }

        current_index = next_index;
    }
}

/* 把扫描期间新注册的任务统一挂到活动链表中。 */
static void mag_scheduler_commit_pending_nodes(void)
{
    while (s_pending_head != MAG_SCHEDULER_NODE_NONE)
    {
        uint8_t node_index = s_pending_head;
        uint8_t next_index = s_task_pool[node_index].next_index;

        s_pending_head = next_index;
        s_task_pool[node_index].next_index = MAG_SCHEDULER_NODE_NONE;

        if (s_task_pool[node_index].state ==
            MAG_SCHEDULER_NODE_REMOVE_PENDING)
        {
            mag_scheduler_release_node(node_index);
        }
        else
        {
            s_task_pool[node_index].state = MAG_SCHEDULER_NODE_ARMED;
            mag_scheduler_link_active_front(node_index);
        }
    }
}

/* 初始化调度器的木牌池、链表和毫秒时基。 */
MAG_SchedulerStatus_e MAG_SchedulerInit(void)
{
    uint8_t node_index;

    if (s_is_processing != 0U)
    {
        return MAG_SCHEDULER_STATUS_BUSY;
    }

    /* 第一步：清空活动链和等待链。 */
    s_is_initialized = 0U;
    s_active_head = MAG_SCHEDULER_NODE_NONE;
    s_pending_head = MAG_SCHEDULER_NODE_NONE;
    s_free_head = 0U;

    /* 第二步：把所有静态木牌依次串成空闲链表。 */
    for (node_index = 0U;
         node_index < (uint8_t)MAG_SCHEDULER_MAX_TASKS;
         node_index++)
    {
        MAG_SchedulerNode_t *p_node = &s_task_pool[node_index];

        p_node->callback = NULL;
        p_node->p_context = NULL;
        p_node->next_due_tick = 0U;
        p_node->period_ms = 0U;
        p_node->remaining_runs = 0U;
        mag_scheduler_advance_generation(p_node);
        p_node->state = MAG_SCHEDULER_NODE_FREE;
        p_node->next_index =
            (node_index + 1U < (uint8_t)MAG_SCHEDULER_MAX_TASKS) ?
            (uint8_t)(node_index + 1U) : MAG_SCHEDULER_NODE_NONE;
    }

    /* 第三步：启动旧 MAG 共用的 1 ms 时基并检查初始化结果。 */
    if (mag_tick_init() != MAG_TICK_INIT_OK)
    {
        return MAG_SCHEDULER_STATUS_TICK_INIT_FAILED;
    }

    s_is_initialized = 1U;
    return MAG_SCHEDULER_STATUS_OK;
}

/* 注册任务并把任务挂到活动链表中。 */
MAG_SchedulerStatus_e MAG_SchedulerRegister(const MAG_TaskConfig_t *p_config,
                                            MAG_TaskHandle_t *p_handle)
{
    uint8_t node_index;
    MAG_SchedulerNode_t *p_node;

    /* 第一步：检查调度器状态和调用参数。 */
    if (s_is_initialized == 0U)
    {
        return MAG_SCHEDULER_STATUS_NOT_INITIALIZED;
    }

    if ((p_handle == NULL) ||
        (mag_scheduler_config_is_valid(p_config) == 0U))
    {
        return MAG_SCHEDULER_STATUS_INVALID_ARGUMENT;
    }

    /* 第二步：从空闲链表取出一块木牌并填写任务信息。 */
    node_index = mag_scheduler_allocate_node();
    if (node_index == MAG_SCHEDULER_NODE_NONE)
    {
        return MAG_SCHEDULER_STATUS_FULL;
    }

    p_node = &s_task_pool[node_index];
    p_node->callback = p_config->callback;
    p_node->p_context = p_config->p_context;
    p_node->next_due_tick = get_mag_tick() + p_config->first_delay_ms;
    p_node->period_ms = p_config->period_ms;
    p_node->remaining_runs = p_config->run_count;

    /* 第三步：扫描期间先挂等待链，其余时间直接挂活动链。 */
    if (s_is_processing != 0U)
    {
        p_node->state = MAG_SCHEDULER_NODE_PENDING_ADD;
        p_node->next_index = s_pending_head;
        s_pending_head = node_index;
    }
    else
    {
        p_node->state = MAG_SCHEDULER_NODE_ARMED;
        mag_scheduler_link_active_front(node_index);
    }

    /* 第四步：返回节点位置和代次组成的有效句柄。 */
    p_handle->index = node_index;
    p_handle->generation = p_node->generation;

    return MAG_SCHEDULER_STATUS_OK;
}

/* 自动遍历四参数任务表，整表失败时撤销本次注册的任务。 */
MAG_SchedulerStatus_e MAG_SchedulerRegisterTable(const MAG_TaskTableEntry_t *p_table,
                                                 uint32_t task_count)
{
    MAG_TaskHandle_t handles[MAG_SCHEDULER_MAX_TASKS];
    MAG_TaskConfig_t config;
    MAG_SchedulerStatus_e status;
    uint32_t index;

    if (s_is_initialized == 0U)
    {
        return MAG_SCHEDULER_STATUS_NOT_INITIALIZED;
    }
    if ((p_table == NULL) || (task_count == 0U))
    {
        return MAG_SCHEDULER_STATUS_INVALID_ARGUMENT;
    }
    if (task_count > MAG_SCHEDULER_MAX_TASKS)
    {
        return MAG_SCHEDULER_STATUS_FULL;
    }
    for (index = 0U; index < task_count; index++)
    {
        config.callback = p_table[index].callback;
        config.p_context = NULL;
        config.period_ms = p_table[index].period_ms;
        config.first_delay_ms = p_table[index].first_delay_ms;
        config.run_count = p_table[index].run_count;
        status = MAG_SchedulerRegister(&config, &handles[index]);
        if (status != MAG_SCHEDULER_STATUS_OK)
        {
            while (index > 0U)
            {
                index--;
                (void)MAG_SchedulerCancel(handles[index]);
            }
            return status;
        }
    }
    return MAG_SCHEDULER_STATUS_OK;
}

/* 取消指定任务并把对应木牌归还空闲链表。 */
MAG_SchedulerStatus_e MAG_SchedulerCancel(MAG_TaskHandle_t handle)
{
    /* 第一步：确认句柄仍然指向同一个有效任务。 */
    if (s_is_initialized == 0U)
    {
        return MAG_SCHEDULER_STATUS_NOT_INITIALIZED;
    }

    if (mag_scheduler_handle_is_valid(handle) == 0U)
    {
        return MAG_SCHEDULER_STATUS_INVALID_HANDLE;
    }

    /* 第二步：扫描期间只做删除标记，避免回调破坏链表。 */
    if (s_is_processing != 0U)
    {
        s_task_pool[handle.index].state =
            MAG_SCHEDULER_NODE_REMOVE_PENDING;
        return MAG_SCHEDULER_STATUS_OK;
    }

    /* 第三步：非扫描期间立即摘链并回收木牌。 */
    if (mag_scheduler_unlink_active(handle.index) == 0U)
    {
        return MAG_SCHEDULER_STATUS_INVALID_HANDLE;
    }

    mag_scheduler_release_node(handle.index);
    return MAG_SCHEDULER_STATUS_OK;
}

/* 从头到尾扫描活动链表并执行已经到期的任务。 */
void MAG_SchedulerProcess(void)
{
    uint8_t previous_index = MAG_SCHEDULER_NODE_NONE;
    uint8_t current_index;

    if ((s_is_initialized == 0U) || (s_is_processing != 0U))
    {
        return;
    }

    s_is_processing = 1U;
    current_index = s_active_head;

    while (current_index != MAG_SCHEDULER_NODE_NONE)
    {
        MAG_SchedulerNode_t *p_node = &s_task_pool[current_index];
        uint8_t next_index = p_node->next_index;
        uint8_t should_remove = 0U;

        /* 第一步：先清理被前一个回调标记删除的当前木牌。 */
        if (p_node->state == MAG_SCHEDULER_NODE_REMOVE_PENDING)
        {
            should_remove = 1U;
        }
        /* 第二步：任务到期后调用函数，函数返回后再处理链表。 */
        else if ((p_node->state == MAG_SCHEDULER_NODE_ARMED) &&
                 (mag_scheduler_time_reached(get_mag_tick(),
                                             p_node->next_due_tick) != 0U))
        {
            MAG_TaskCallback_t callback = p_node->callback;
            void *p_context = p_node->p_context;

            p_node->state = MAG_SCHEDULER_NODE_RUNNING;
            callback(p_context);

            if (p_node->state == MAG_SCHEDULER_NODE_REMOVE_PENDING)
            {
                should_remove = 1U;
            }
            else
            {
                /* 第三步：有限任务执行一次后减少一次剩余次数。 */
                if (p_node->remaining_runs != MAG_SCHEDULER_RUN_FOREVER)
                {
                    p_node->remaining_runs--;
                }

                /* 第四步：次数用完就摘牌，否则更新下次执行时刻。 */
                if (p_node->remaining_runs == 0U)
                {
                    should_remove = 1U;
                }
                else
                {
                    p_node->next_due_tick =
                        get_mag_tick() + p_node->period_ms;
                    p_node->state = MAG_SCHEDULER_NODE_ARMED;
                }
            }
        }

        /* 第五步：根据执行结果保留当前木牌或者把它归还空闲链。 */
        if (should_remove != 0U)
        {
            if (previous_index == MAG_SCHEDULER_NODE_NONE)
            {
                s_active_head = next_index;
            }
            else
            {
                s_task_pool[previous_index].next_index = next_index;
            }

            mag_scheduler_release_node(current_index);
        }
        else
        {
            previous_index = current_index;
        }

        current_index = next_index;
    }

    /* 第六步：统一提交回调中产生的删除和新增请求。 */
    mag_scheduler_cleanup_removed_nodes();
    mag_scheduler_commit_pending_nodes();
    s_is_processing = 0U;
}
