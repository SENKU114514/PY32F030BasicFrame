#ifndef EXTERN_HW_INIT_LOWPOWER_H
#define EXTERN_HW_INIT_LOWPOWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "./HW_INIT/GPIO/GPIO_init.h"

/* 唤醒按键配置：默认B2未被V0.7外设占用，若改为A5则需停用或改走USART2_RX。 */
#define LOWPOWER_WAKEUP_KEY_PIN            B2
#define LOWPOWER_WAKEUP_KEY_PULL           UP
#define LOWPOWER_WAKEUP_KEY_TRIGGER        GPIO_IT_FALLING

/* Stop档位配置：Stop0唤醒快，Stop1功耗更低。 */
#define LOWPOWER_STOP_MODE_STOP0           0U
#define LOWPOWER_STOP_MODE_STOP1_1V2       1U
#define LOWPOWER_STOP_MODE_STOP1_1V0       2U
#define LOWPOWER_STOP_MODE                 LOWPOWER_STOP_MODE_STOP1_1V0

/* 唤醒后恢复的系统主频。 */
#define LOWPOWER_RESTORE_CLOCK_MHZ         48U

/* 外设管理开关：1表示暂停、清旧事件并按原状态恢复，0表示低功耗模块不处理。 */
#define LOWPOWER_MANAGE_TIM1               1U
#define LOWPOWER_MANAGE_TIM3               1U
#define LOWPOWER_MANAGE_TIM14              1U
#define LOWPOWER_MANAGE_TIM16              1U
#define LOWPOWER_MANAGE_TIM17              1U
#define LOWPOWER_MANAGE_ADC1               1U
#define LOWPOWER_MANAGE_SPI1               1U
#define LOWPOWER_MANAGE_I2C1               1U
#define LOWPOWER_MANAGE_USART1             1U
#define LOWPOWER_MANAGE_USART2             1U
#define LOWPOWER_MANAGE_DMA1               1U

/* 若外设本身承担唤醒任务则把对应开关设为0；IWDG启动后无法暂停，休眠时间必须小于喂狗周期。 */

#if ((LOWPOWER_STOP_MODE != LOWPOWER_STOP_MODE_STOP0) && \
     (LOWPOWER_STOP_MODE != LOWPOWER_STOP_MODE_STOP1_1V2) && \
     (LOWPOWER_STOP_MODE != LOWPOWER_STOP_MODE_STOP1_1V0))
#error "LOWPOWER_STOP_MODE is invalid"
#endif

typedef enum
{
    HW_LOWPOWER_STATUS_OK = 0,
    HW_LOWPOWER_STATUS_WAKEUP_CONFIG_FAILED,
    HW_LOWPOWER_STATUS_INTERRUPTS_DISABLED,
    HW_LOWPOWER_STATUS_HANDLER_MODE,
    HW_LOWPOWER_STATUS_PERIPHERAL_BUSY,
    HW_LOWPOWER_STATUS_SUSPEND_FAILED,
} HW_LowPower_Status_e;

/* 休眠前处理板级负载，返回0可取消本次休眠。 */
uint8_t HW_LowPower_PrepareCallback(void);

/* 唤醒后恢复板级负载，该回调执行时全局中断仍处于关闭状态。 */
void HW_LowPower_RestoreCallback(void);

/* 自动暂停已选择的外设并进入Stop，唤醒且恢复完成后才返回。 */
HW_LowPower_Status_e HW_LowPower_Enter(void);

#ifdef __cplusplus
}
#endif

#endif /* EXTERN_HW_INIT_LOWPOWER_H */
