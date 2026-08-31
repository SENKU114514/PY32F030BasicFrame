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
    MAG_TaskCallback_t callback;
    void *p_context;
    uint32_t first_delay_ms;
    uint32_t period_ms;
    uint32_t run_count;
} MAG_TaskConfig_t;

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

/* 初始化调度器的木牌池、链表和毫秒时基。 */
MAG_SchedulerStatus_e MAG_SchedulerInit(void);

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
