#ifndef __TIM_INIT_H__
#define __TIM_INIT_H__

#include "main.h"

/*
 * PWM 输出通道。枚举名同时保存 TIM、通道、GPIO 引脚，避免只传 CH1 时
 * 无法区分同一通道的多组复用引脚。实际使用前仍须确认所选封装引出了该脚。
 */
typedef enum
{
#ifdef TIM1
    PWM_TIM1_CH1_A3,      /* PA3  -> TIM1_CH1, AF13 */
    PWM_TIM1_CH1_A8,      /* PA8  -> TIM1_CH1, AF2  */
    PWM_TIM1_CH2_A9,      /* PA9  -> TIM1_CH2, AF2  */
    PWM_TIM1_CH2_A13,     /* PA13 -> TIM1_CH2, AF13；会占用 SWDIO */
    PWM_TIM1_CH2_B3,      /* PB3  -> TIM1_CH2, AF1  */
    PWM_TIM1_CH3_A0,      /* PA0  -> TIM1_CH3, AF13 */
    PWM_TIM1_CH3_A10,     /* PA10 -> TIM1_CH3, AF2  */
    PWM_TIM1_CH3_B6,      /* PB6  -> TIM1_CH3, AF1  */
    PWM_TIM1_CH4_A1,      /* PA1  -> TIM1_CH4, AF13 */
    PWM_TIM1_CH4_A11,     /* PA11 -> TIM1_CH4, AF2  */
#endif

#ifdef TIM3
    PWM_TIM3_CH1_A2,      /* PA2  -> TIM3_CH1, AF13 */
    PWM_TIM3_CH1_A6,      /* PA6  -> TIM3_CH1, AF1  */
    PWM_TIM3_CH1_B4,      /* PB4  -> TIM3_CH1, AF1  */
    PWM_TIM3_CH2_A5,      /* PA5  -> TIM3_CH2, AF13 */
    PWM_TIM3_CH2_A7,      /* PA7  -> TIM3_CH2, AF1  */
    PWM_TIM3_CH2_B5,      /* PB5  -> TIM3_CH2, AF1  */
    PWM_TIM3_CH3_A4,      /* PA4  -> TIM3_CH3, AF13 */
    PWM_TIM3_CH3_B0,      /* PB0  -> TIM3_CH3, AF1  */
    PWM_TIM3_CH3_F3,      /* PF3  -> TIM3_CH3, AF13 */
    PWM_TIM3_CH4_B1,      /* PB1  -> TIM3_CH4, AF1  */
#endif

#ifdef TIM14
    PWM_TIM14_CH1_A4,     /* PA4  -> TIM14_CH1, AF4  */
    PWM_TIM14_CH1_A7,     /* PA7  -> TIM14_CH1, AF4  */
    PWM_TIM14_CH1_B1,     /* PB1  -> TIM14_CH1, AF0  */
    PWM_TIM14_CH1_F0,     /* PF0  -> TIM14_CH1, AF2；使用 HSE 时不能同时占用 */
    PWM_TIM14_CH1_F1,     /* PF1  -> TIM14_CH1, AF13；使用 HSE 时不能同时占用 */
#endif

#ifdef TIM16
    PWM_TIM16_CH1_A6,     /* PA6  -> TIM16_CH1, AF5 */
    PWM_TIM16_CH1_B8,     /* PB8  -> TIM16_CH1, AF2 */
#endif

#ifdef TIM17
    PWM_TIM17_CH1_A7,     /* PA7  -> TIM17_CH1, AF5  */
    PWM_TIM17_CH1_B8,     /* PB8  -> TIM17_CH1, AF13 */
#endif

    PWM_CHANNEL_COUNT,
} TIM_PWM_Channel_e;

/* 定时器硬件层返回状态，供 Service / APP 层判断初始化结果。 */
typedef enum
{
    HW_TIM_STATUS_OK = 0,
    HW_TIM_STATUS_INVALID_ARG,
    HW_TIM_STATUS_UNSUPPORTED,
    HW_TIM_STATUS_GPIO_INIT_FAILED,
    HW_TIM_STATUS_PERIOD_OUT_OF_RANGE,
    HW_TIM_STATUS_FREQUENCY_OUT_OF_RANGE,
    HW_TIM_STATUS_INIT_FAILED,
    HW_TIM_STATUS_BUSY,
    HW_TIM_STATUS_NOT_INITIALIZED,
    HW_TIM_STATUS_NOT_STARTED,
    HW_TIM_STATUS_OVERFLOW,
} HW_TIM_Status_e;

//中断返回
__weak void TIM1_UpdateCallback	(void);
__weak void TIM3_UpdateCallback	(void);
__weak void TIM14_UpdateCallback(void);
__weak void TIM16_UpdateCallback(void);
__weak void TIM17_UpdateCallback(void);


/*
 * 初始化基础定时器更新中断。
 * TIMx 传入 TIM1、TIM3、TIM14、TIM16 或 TIM17；计数方向传入 UP 或 DOWN；
 * period_ms 为中断周期（毫秒）。PSC 与 ARR 由当前 APB 时钟自动计算。
 */
HW_TIM_Status_e HW_TIM_COUNT_IT_init(TIM_TypeDef *TIMx,
                                     uint8_t count_mode,
                                     uint32_t period_ms);

/*
 * 初始化 PWM 输出。
 * 参数顺序：TIMx、PWM 通道/引脚、频率（Hz）、占空比（0~100）。
 * 内部自动配置 GPIO 复用、TIM 时钟、PSC、ARR 与比较值。
 */
HW_TIM_Status_e HW_TIM_PWM_init(TIM_TypeDef *TIMx,
                                TIM_PWM_Channel_e channel,
                                uint32_t frequency_hz,
                                uint8_t duty_percent);





/* 手动计时使用约定，US 与 MS 接口必须成组使用。 */
/*
 * 1. TIMx 必须空闲且由计时功能独占；框架的 TIM1 已用于 MAG 时基。
 * 2. US 的量程为 0~65535 微秒，MS 的量程为 0~65535 毫秒。
 * 3. 时钟不能准确分出所选单位时返回频率错误，不启用溢出中断。
 * 4. 同一定时器的接口不能交叉执行；开始和停止可在 GPIO 中断中调用。
 */
/* 初始化微秒计时，量程 0~65535 微秒，初始化后保持停止。 */
HW_TIM_Status_e HW_TIM_TIME_US_init(TIM_TypeDef *TIMx);
/* 清零并开始微秒计时，重复调用会重新开始。 */
HW_TIM_Status_e HW_TIM_TIME_US_Start(TIM_TypeDef *TIMx);
/* 停止微秒计时，通过 elapsed_us 输出耗时，返回执行状态。 */
HW_TIM_Status_e HW_TIM_TIME_US_Stop(TIM_TypeDef *TIMx, uint32_t *elapsed_us);

/* 初始化毫秒计时，量程 0~65535 毫秒，初始化后保持停止。 */
HW_TIM_Status_e HW_TIM_TIME_MS_init(TIM_TypeDef *TIMx);
/* 清零并开始毫秒计时，重复调用会重新开始。 */
HW_TIM_Status_e HW_TIM_TIME_MS_Start(TIM_TypeDef *TIMx);
/* 停止毫秒计时，通过 elapsed_ms 输出耗时，返回执行状态。 */
HW_TIM_Status_e HW_TIM_TIME_MS_Stop(TIM_TypeDef *TIMx, uint32_t *elapsed_ms);

/*
 * 1. 停止时不清零，重复停止可读取同一结果，下次开始时才清零。
 * 2. 未开始返回 NOT_STARTED；超量程返回 OVERFLOW；单位不匹配返回 INVALID_ARG。
 * 3. 仅 OK 时写入输出变量，发生错误时保持输出变量不变。
 */


#endif


