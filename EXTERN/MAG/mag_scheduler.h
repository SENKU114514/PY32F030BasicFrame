#ifndef EXTERN_MAG_SCHEDULER_H
#define EXTERN_MAG_SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 调度器最多同时保存的任务数量。 */
#ifndef MAG_SCHEDULER_MAX_TASKS
#define MAG_SCHEDULER_MAX_TASKS          8U
#endif

/* 任务永久循环时使用的运行次数。 */
#define MAG_SCHEDULER_RUN_FOREVER        ((uint32_t)0xFFFFFFFFUL)

/* 调度任务的回调函数类型。 */
typedef void (*MAG_TaskCallback_t)(void *p_context);

/* 注册一个调度任务时需要填写的参数。 */
typedef struct
{
    MAG_TaskCallback_t callback; // 任务执行函数
    void *p_context;             // 传给任务函数的参数，不需要时填 NULL
    uint32_t first_delay_ms;     // 注册后首次执行的等待时间，单位 ms
    uint32_t period_ms;          // 每次任务返回后到下次执行的间隔，单位 ms
    uint32_t run_count;          // 执行次数，MAG_SCHEDULER_RUN_FOREVER 表示永久循环
} MAG_TaskConfig_t;

/* 静态任务表：每行只填 {执行函数, 执行间隔ms, 首次延迟ms, 执行次数}。 */
typedef struct
{
    MAG_TaskCallback_t callback; /* void task(void *p_context)，参数固定传 NULL。 */
    uint32_t period_ms;         /* 本次返回后等待多久再执行；不是函数运行时长。 */
    uint32_t first_delay_ms;    /* 注册后首次等待时间；0 表示尽快执行。 */
    uint32_t run_count;         /* 1=一次，N=N次，MAG_SCHEDULER_RUN_FOREVER=永久。 */
} MAG_TaskTableEntry_t;

/* 调度任务句柄，由节点位置和节点代次共同确定。 */
typedef struct
{
    uint32_t generation;
    uint8_t index;
} MAG_TaskHandle_t;

/* 调度器公共接口的返回状态。 */
typedef enum
{
    MAG_SCHEDULER_STATUS_OK = 0,
    MAG_SCHEDULER_STATUS_INVALID_ARGUMENT,
    MAG_SCHEDULER_STATUS_NOT_INITIALIZED,
    MAG_SCHEDULER_STATUS_FULL,
    MAG_SCHEDULER_STATUS_INVALID_HANDLE,
    MAG_SCHEDULER_STATUS_TICK_INIT_FAILED,
    MAG_SCHEDULER_STATUS_BUSY,
} MAG_SchedulerStatus_e;

/*
 * 初始化调度器并自动注册整张任务表，在业务模块初始化完成后调用一次。
 * p_table：任务表首地址；task_count：任务数量，sizeof(table) / sizeof(table[0])。
 * 用法：MAG_SchedulerInit(s_app_tasks, sizeof(s_app_tasks) / sizeof(s_app_tasks[0]));
 * 表不能为空，数量为 1 到 MAG_SCHEDULER_MAX_TASKS；表内容在注册时复制。
 * 时间单位为 ms，首次延迟和间隔最大为 0x7FFFFFFF；首次延迟从注册时计时。
 * 次数不能为 0；重复任务的间隔必须大于 0，仅执行一次时允许间隔为 0。
 * 返回 OK 表示初始化及整表注册均成功，失败后不得开始正常调度，需重新初始化。
 * 重新初始化会清空旧任务；调度过程中调用返回 BUSY，不改变现有任务。
 * 不在中断中调用；任务函数不会在初始化期间执行。
 */
MAG_SchedulerStatus_e MAG_SchedulerInit(const MAG_TaskTableEntry_t *p_table,
                                      uint32_t task_count);

/* 在主循环或任务回调中注册任务，不能从中断服务中调用。 */
MAG_SchedulerStatus_e MAG_SchedulerRegister(const MAG_TaskConfig_t *p_config,
                                            MAG_TaskHandle_t *p_handle);

/* 在主循环或任务回调中取消任务，不能从中断服务中调用。 */
MAG_SchedulerStatus_e MAG_SchedulerCancel(MAG_TaskHandle_t handle);

/* 在主循环中扫描活动链表，任务回调必须短小且不能阻塞。 */
void MAG_SchedulerProcess(void);

#ifdef __cplusplus
}
#endif

#endif /* EXTERN_MAG_SCHEDULER_H */
