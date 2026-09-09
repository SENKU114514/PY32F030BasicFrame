#ifndef EXTERN_HW_INIT_LOWPOWER_H
#define EXTERN_HW_INIT_LOWPOWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "./HW_INIT/GPIO/GPIO_init.h"

typedef enum
{
    HW_LOWPOWER_STATUS_OK = 0,
    HW_LOWPOWER_STATUS_WAKEUP_CONFIG_FAILED,
    HW_LOWPOWER_STATUS_INTERRUPTS_DISABLED,
    HW_LOWPOWER_STATUS_HANDLER_MODE,
    HW_LOWPOWER_STATUS_PERIPHERAL_BUSY,
    HW_LOWPOWER_STATUS_SUSPEND_FAILED,
    HW_LOWPOWER_STATUS_NOT_INITIALIZED,
    HW_LOWPOWER_STATUS_INVALID_ARG,
    HW_LOWPOWER_STATUS_UNSUPPORTED_CLOCK,
    HW_LOWPOWER_STATUS_KEY_READ_FAILED,
} HW_LowPower_Status_e;

/* 休眠前处理板级负载，返回0可取消本次休眠。 */
uint8_t HW_LowPower_PrepareCallback(void);

/* 恢复板级负载或撤销休眠准备；回调不可依赖中断或使用阻塞等待。 */
void HW_LowPower_RestoreCallback(void);

/* 初始化唤醒按键，指定引脚、上下拉和按下时的有效电平 LOW/HIGH。 */
HW_LowPower_Status_e HW_LowPower_init(GPIO_index_e pin, GPIO_pull_e pull,
                                     GPIO_mode_e active_level);

/* 暂停普通外设并进入 Stop1，长按 3 秒后恢复系统并返回。 */
/*
 * 1. 在主循环调用；只有唤醒确认阶段使用 1MHz，Stop 期间高速时钟停止。
 * 2. 短按继续休眠；进入时已按下则先松手，再接受下一次长按。
 * 3. 唤醒按键使用双边沿，模块内部完成确认，不需要 APP 按键 FSM。
 * 4. 恢复原 4/8/16/24/48MHz 时钟及外设，MAG tick 不累计休眠时间。
 * 5. 外设忙或无法暂停时返回 BUSY / SUSPEND_FAILED，不强行破坏正在进行的传输。
 * 6. 不关闭板外负载，使用 Prepare/Restore 回调处理；保留 GPIO 输出状态。
 * 7. 已启动 IWDG 时不能无限休眠，必须先规划看门狗策略；本接口不暂停 IWDG。
 * 8. 返回时按键可能仍按住，APP 如需屏蔽该次业务按键，应等松手后重置其 FSM。
 * 9. 初始化会配置该引脚的 EXTI，调用者须选用未被其他功能占用的引脚。
 */
HW_LowPower_Status_e HW_LowPower_Enter(void);

#ifdef __cplusplus
}
#endif

#endif /* EXTERN_HW_INIT_LOWPOWER_H */
