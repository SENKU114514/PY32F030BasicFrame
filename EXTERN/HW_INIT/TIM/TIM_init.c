#include "./HW_INIT/TIM/TIM_init.h"

/*
 * TIM 使用示例（各模式独立使用，同一定时器不能同时承担多种功能）：
 *
 * 1. 周期中断：向下计数，每 1ms 进入 TIM3_UpdateCallback()。
 *    status = HW_TIM_COUNT_IT_init(TIM3, DOWN, 1U);
 *
 * 2. PWM 输出：TIM3 的 CH1 使用 A6，频率 1000Hz，占空比 50%。
 *    status = HW_TIM_PWM_init(TIM3, PWM_TIM3_CH1_A6, 1000U, 50U);
 *
 * 3. 手动计时：以下三步在对应的初始化、开始和结束位置分别调用。
 *    uint32_t elapsed = 0U;
 *    HW_TIM_Status_e status;
 *    status = HW_TIM_TIME_US_init(TIM3);
 *    // 初始化成功后，在需要开始的位置调用：
 *    status = HW_TIM_TIME_US_Start(TIM3);
 *    // 开始成功后，在需要结束的位置调用：
 *    status = HW_TIM_TIME_US_Stop(TIM3, &elapsed);
 *    // status 为 OK 时，elapsed 为本次耗时，单位为微秒。
 *
 * 4. 毫秒计时：在对应位置依次调用，检查成功后再进行下一步。
 *    status = HW_TIM_TIME_MS_init(TIM3);
 *    status = HW_TIM_TIME_MS_Start(TIM3);
 *    status = HW_TIM_TIME_MS_Stop(TIM3, &elapsed);
 *    // status 为 OK 时，elapsed 为本次耗时，单位为毫秒。
 *    // 两组接口不能混用，切换单位前必须先停止，再重新初始化。
 *
 * 注意：每次调用均需检查 status，成功后再进行下一步。
 *       开始时清零，停止时保留结果；量程为 0~65535 个所选单位。
 *       OVERFLOW 表示超出量程，本次输出无效，应重新开始计时。
 *       示例假设 TIM3 空闲；框架 TIM1 已用于 MAG，不要占用。
 */

/* PWM 默认电气参数：推挽、无上下拉、高速输出、有效高电平、PWM1 模式。 */
#define HW_TIM_PWM_GPIO_SPEED       LL_GPIO_SPEED_FREQ_HIGH
#define HW_TIM_PWM_OC_POLARITY      LL_TIM_OCPOLARITY_HIGH
#define HW_TIM_PWM_OC_IDLE_STATE    LL_TIM_OCIDLESTATE_LOW
#define HW_TIM_PWM_OC_MODE          LL_TIM_OCMODE_PWM1

typedef struct
{
    TIM_TypeDef *tim_instance;
    uint32_t tim_channel;
    GPIO_TypeDef *gpio_port;
    uint32_t gpio_pin;
    uint32_t gpio_alternate;
    uint8_t main_output_enable;
} HW_TIM_PWM_Map_t;

/* PWM 专用映射表：索引由 TIM_PWM_Channel_e 提供。 */
static const HW_TIM_PWM_Map_t HW_TIM_PWM_MAP[PWM_CHANNEL_COUNT] =
{
#ifdef TIM1
    [PWM_TIM1_CH1_A3]  = {TIM1, LL_TIM_CHANNEL_CH1, GPIOA, LL_GPIO_PIN_3,  LL_GPIO_AF_13, 1U},
    [PWM_TIM1_CH1_A8]  = {TIM1, LL_TIM_CHANNEL_CH1, GPIOA, LL_GPIO_PIN_8,  LL_GPIO_AF_2,  1U},
    [PWM_TIM1_CH2_A9]  = {TIM1, LL_TIM_CHANNEL_CH2, GPIOA, LL_GPIO_PIN_9,  LL_GPIO_AF_2,  1U},
    [PWM_TIM1_CH2_A13] = {TIM1, LL_TIM_CHANNEL_CH2, GPIOA, LL_GPIO_PIN_13, LL_GPIO_AF_13, 1U},
    [PWM_TIM1_CH2_B3]  = {TIM1, LL_TIM_CHANNEL_CH2, GPIOB, LL_GPIO_PIN_3,  LL_GPIO_AF_1,  1U},
    [PWM_TIM1_CH3_A0]  = {TIM1, LL_TIM_CHANNEL_CH3, GPIOA, LL_GPIO_PIN_0,  LL_GPIO_AF_13, 1U},
    [PWM_TIM1_CH3_A10] = {TIM1, LL_TIM_CHANNEL_CH3, GPIOA, LL_GPIO_PIN_10, LL_GPIO_AF_2,  1U},
    [PWM_TIM1_CH3_B6]  = {TIM1, LL_TIM_CHANNEL_CH3, GPIOB, LL_GPIO_PIN_6,  LL_GPIO_AF_1,  1U},
    [PWM_TIM1_CH4_A1]  = {TIM1, LL_TIM_CHANNEL_CH4, GPIOA, LL_GPIO_PIN_1,  LL_GPIO_AF_13, 1U},
    [PWM_TIM1_CH4_A11] = {TIM1, LL_TIM_CHANNEL_CH4, GPIOA, LL_GPIO_PIN_11, LL_GPIO_AF_2,  1U},
#endif

#ifdef TIM3
    [PWM_TIM3_CH1_A2]  = {TIM3, LL_TIM_CHANNEL_CH1, GPIOA, LL_GPIO_PIN_2,  LL_GPIO_AF_13, 0U},
    [PWM_TIM3_CH1_A6]  = {TIM3, LL_TIM_CHANNEL_CH1, GPIOA, LL_GPIO_PIN_6,  LL_GPIO_AF_1,  0U},
    [PWM_TIM3_CH1_B4]  = {TIM3, LL_TIM_CHANNEL_CH1, GPIOB, LL_GPIO_PIN_4,  LL_GPIO_AF_1,  0U},
    [PWM_TIM3_CH2_A5]  = {TIM3, LL_TIM_CHANNEL_CH2, GPIOA, LL_GPIO_PIN_5,  LL_GPIO_AF_13, 0U},
    [PWM_TIM3_CH2_A7]  = {TIM3, LL_TIM_CHANNEL_CH2, GPIOA, LL_GPIO_PIN_7,  LL_GPIO_AF_1,  0U},
    [PWM_TIM3_CH2_B5]  = {TIM3, LL_TIM_CHANNEL_CH2, GPIOB, LL_GPIO_PIN_5,  LL_GPIO_AF_1,  0U},
    [PWM_TIM3_CH3_A4]  = {TIM3, LL_TIM_CHANNEL_CH3, GPIOA, LL_GPIO_PIN_4,  LL_GPIO_AF_13, 0U},
    [PWM_TIM3_CH3_B0]  = {TIM3, LL_TIM_CHANNEL_CH3, GPIOB, LL_GPIO_PIN_0,  LL_GPIO_AF_1,  0U},
    [PWM_TIM3_CH3_F3]  = {TIM3, LL_TIM_CHANNEL_CH3, GPIOF, LL_GPIO_PIN_3,  LL_GPIO_AF_13, 0U},
    [PWM_TIM3_CH4_B1]  = {TIM3, LL_TIM_CHANNEL_CH4, GPIOB, LL_GPIO_PIN_1,  LL_GPIO_AF_1,  0U},
#endif

#ifdef TIM14
    [PWM_TIM14_CH1_A4] = {TIM14, LL_TIM_CHANNEL_CH1, GPIOA, LL_GPIO_PIN_4, LL_GPIO_AF_4,  0U},
    [PWM_TIM14_CH1_A7] = {TIM14, LL_TIM_CHANNEL_CH1, GPIOA, LL_GPIO_PIN_7, LL_GPIO_AF_4,  0U},
    [PWM_TIM14_CH1_B1] = {TIM14, LL_TIM_CHANNEL_CH1, GPIOB, LL_GPIO_PIN_1, LL_GPIO_AF_0,  0U},
    [PWM_TIM14_CH1_F0] = {TIM14, LL_TIM_CHANNEL_CH1, GPIOF, LL_GPIO_PIN_0, LL_GPIO_AF_2,  0U},
    [PWM_TIM14_CH1_F1] = {TIM14, LL_TIM_CHANNEL_CH1, GPIOF, LL_GPIO_PIN_1, LL_GPIO_AF_13, 0U},
#endif

#ifdef TIM16
    [PWM_TIM16_CH1_A6] = {TIM16, LL_TIM_CHANNEL_CH1, GPIOA, LL_GPIO_PIN_6, LL_GPIO_AF_5, 1U},
    [PWM_TIM16_CH1_B8] = {TIM16, LL_TIM_CHANNEL_CH1, GPIOB, LL_GPIO_PIN_8, LL_GPIO_AF_2, 1U},
#endif

#ifdef TIM17
    [PWM_TIM17_CH1_A7] = {TIM17, LL_TIM_CHANNEL_CH1, GPIOA, LL_GPIO_PIN_7, LL_GPIO_AF_5,  1U},
    [PWM_TIM17_CH1_B8] = {TIM17, LL_TIM_CHANNEL_CH1, GPIOB, LL_GPIO_PIN_8, LL_GPIO_AF_13, 1U},
#endif
};

/* 根据 TIM 实例使能外设时钟，并取回对应的更新中断入口。 */
static HW_TIM_Status_e HW_TIM_EnableClockAndGetIRQ(TIM_TypeDef *TIMx,
                                                    IRQn_Type *irqn)
{
    if (TIMx == NULL)
    {
        return HW_TIM_STATUS_INVALID_ARG;
    }

#ifdef TIM1
    if (TIMx == TIM1)
    {
        LL_APB1_GRP2_EnableClock(RCC_APBENR2_TIM1EN);
        if (irqn != NULL)
        {
            *irqn = TIM1_BRK_UP_TRG_COM_IRQn;
        }
        return HW_TIM_STATUS_OK;
    }
#endif

#ifdef TIM3
    if (TIMx == TIM3)
    {
        LL_APB1_GRP1_EnableClock(RCC_APBENR1_TIM3EN);
        if (irqn != NULL)
        {
            *irqn = TIM3_IRQn;
        }
        return HW_TIM_STATUS_OK;
    }
#endif

#ifdef TIM14
    if (TIMx == TIM14)
    {
        LL_APB1_GRP2_EnableClock(RCC_APBENR2_TIM14EN);
        if (irqn != NULL)
        {
            *irqn = TIM14_IRQn;
        }
        return HW_TIM_STATUS_OK;
    }
#endif

#ifdef TIM16
    if (TIMx == TIM16)
    {
        LL_APB1_GRP2_EnableClock(RCC_APBENR2_TIM16EN);
        if (irqn != NULL)
        {
            *irqn = TIM16_IRQn;
        }
        return HW_TIM_STATUS_OK;
    }
#endif

#ifdef TIM17
    if (TIMx == TIM17)
    {
        LL_APB1_GRP2_EnableClock(RCC_APBENR2_TIM17EN);
        if (irqn != NULL)
        {
            *irqn = TIM17_IRQn;
        }
        return HW_TIM_STATUS_OK;
    }
#endif

    return HW_TIM_STATUS_UNSUPPORTED;
}


/* 计时单位，停止时输出的耗时与初始化选择的单位一致。 */
typedef enum
{
    HW_TIM_TIME_UNIT_US = 0,
    HW_TIM_TIME_UNIT_MS,
} HW_TIM_TimeUnit_e;

/* 保存每个定时器的计时状态。 */
typedef struct
{
    TIM_TypeDef *instance; // 定时器实例
    HW_TIM_TimeUnit_e unit; // 当前计时单位，防止两组接口混用
    uint8_t initialized;   // 初始化完成标志
    uint8_t started;       // 是否开始过计时，停止后保留
} HW_TIM_TimeState_t;

static HW_TIM_TimeState_t s_time_states[] =
{
#ifdef TIM1
    {TIM1, HW_TIM_TIME_UNIT_US, 0U, 0U},
#endif
#ifdef TIM3
    {TIM3, HW_TIM_TIME_UNIT_US, 0U, 0U},
#endif
#ifdef TIM14
    {TIM14, HW_TIM_TIME_UNIT_US, 0U, 0U},
#endif
#ifdef TIM16
    {TIM16, HW_TIM_TIME_UNIT_US, 0U, 0U},
#endif
#ifdef TIM17
    {TIM17, HW_TIM_TIME_UNIT_US, 0U, 0U},
#endif
};

/* 根据定时器实例查找对应的计时状态。 */
static HW_TIM_TimeState_t *HW_TIM_TimeState(TIM_TypeDef *TIMx)
{
    uint32_t i;
    for (i = 0U; i < sizeof(s_time_states) / sizeof(s_time_states[0]); i++)
    {
        if (s_time_states[i].instance == TIMx)
            return &s_time_states[i];
    }
    return NULL;
}

/* 初始化手动计时，配置计时单位并保持停止。 */
static HW_TIM_Status_e HW_TIM_TimeInit(TIM_TypeDef *TIMx, HW_TIM_TimeUnit_e unit)
{
    HW_TIM_TimeState_t *state = HW_TIM_TimeState(TIMx);
    LL_RCC_ClocksTypeDef clocks;
    LL_TIM_InitTypeDef config = {0};
    HW_TIM_Status_e status;
    uint32_t frequency;
    uint32_t divider;

    /* 第一步：检查参数和当前计时状态。 */
    if ((TIMx == NULL) || ((unit != HW_TIM_TIME_UNIT_US) &&
                              (unit != HW_TIM_TIME_UNIT_MS)))
        return HW_TIM_STATUS_INVALID_ARG;
    if (state == NULL)
        return HW_TIM_STATUS_UNSUPPORTED;

    /* 第二步：按当前 APB1 = HCLK 的时钟配置计算分频，保证计时单位准确。 */
    LL_RCC_GetSystemClocksFreq(&clocks);
    frequency = (unit == HW_TIM_TIME_UNIT_US) ? 1000000U : 1000U;
    divider = clocks.PCLK1_Frequency / frequency;
    if ((divider == 0U) || (divider > 65536U) ||
        ((clocks.PCLK1_Frequency % frequency) != 0U))
        return HW_TIM_STATUS_FREQUENCY_OUT_OF_RANGE;

    /* 第三步：使能时钟，检查定时器是否已被其他功能占用。 */
    status = HW_TIM_EnableClockAndGetIRQ(TIMx, NULL);
    if (status != HW_TIM_STATUS_OK)
        return status;
    if (LL_TIM_IsEnabledCounter(TIMx) || (TIMx->CCER != 0U) ||
        (TIMx->DIER != 0U))
        return HW_TIM_STATUS_BUSY;

    /* 第四步：复位外设，配置向上计数和 65535 的计数上限。 */
    config.Prescaler = divider - 1U;
    config.Autoreload = 65535U;
    config.CounterMode = LL_TIM_COUNTERMODE_UP;
    config.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    config.RepetitionCounter = 0U;
    state->initialized = 0U;
    if (LL_TIM_DeInit(TIMx) != SUCCESS)
        return HW_TIM_STATUS_INIT_FAILED;
    if (LL_TIM_Init(TIMx, &config) != SUCCESS)
        return HW_TIM_STATUS_INIT_FAILED;
    /* 第五步：装载分频参数并清零，记录初始化完成，等待手动开始。 */
    LL_TIM_GenerateEvent_UPDATE(TIMx);
    LL_TIM_ClearFlag_UPDATE(TIMx);
    LL_TIM_SetCounter(TIMx, 0U);
    state->started = 0U;
    state->unit = unit;
    state->initialized = 1U;
    return HW_TIM_STATUS_OK;
}

/* 清零计数和溢出标志，开始本次计时。 */
static HW_TIM_Status_e HW_TIM_TimeStart(TIM_TypeDef *TIMx, HW_TIM_TimeUnit_e unit)
{
    HW_TIM_TimeState_t *state = HW_TIM_TimeState(TIMx);
    /* 第一步：检查参数和当前计时状态。 */
    if (TIMx == NULL)
        return HW_TIM_STATUS_INVALID_ARG;
    if (state == NULL)
        return HW_TIM_STATUS_UNSUPPORTED;
    if (state->initialized == 0U)
        return HW_TIM_STATUS_NOT_INITIALIZED;
    /* 检查单位，防止微秒与毫秒接口混用。 */
    if (state->unit != unit)
        return HW_TIM_STATUS_INVALID_ARG;

    /* 第二步：停止计数，清零计数值和预分频进度，并清除上次溢出标志。 */
    LL_TIM_DisableCounter(TIMx);
    LL_TIM_GenerateEvent_UPDATE(TIMx);
    LL_TIM_SetCounter(TIMx, 0U);
    LL_TIM_ClearFlag_UPDATE(TIMx);
    /* 第三步：记录已经开始，并启动计数器。 */
    state->started = 1U;
    LL_TIM_EnableCounter(TIMx);
    return HW_TIM_STATUS_OK;
}

/* 停止计时，检查量程并输出本次耗时。 */
static HW_TIM_Status_e HW_TIM_TimeStop(TIM_TypeDef *TIMx, HW_TIM_TimeUnit_e unit, uint32_t *elapsed)
{
    HW_TIM_TimeState_t *state = HW_TIM_TimeState(TIMx);
    /* 第一步：检查参数和当前计时状态。 */
    if ((TIMx == NULL) || (elapsed == NULL))
        return HW_TIM_STATUS_INVALID_ARG;
    if (state == NULL)
        return HW_TIM_STATUS_UNSUPPORTED;
    if (state->initialized == 0U)
        return HW_TIM_STATUS_NOT_INITIALIZED;
    /* 检查单位，防止微秒与毫秒接口混用。 */
    if (state->unit != unit)
        return HW_TIM_STATUS_INVALID_ARG;
    if (state->started == 0U)
        return HW_TIM_STATUS_NOT_STARTED;

    /* 第二步：先停止计数，使本次测量结果固定。 */
    LL_TIM_DisableCounter(TIMx);
    /* 第三步：检查是否超出量程；溢出时保留原输出变量，不返回回绕后的数值。 */
    if (LL_TIM_IsActiveFlag_UPDATE(TIMx))
        return HW_TIM_STATUS_OVERFLOW;
    /* 第四步：按初始化选择的单位输出结果，保留计数值供重复读取。 */
    *elapsed = LL_TIM_GetCounter(TIMx);
    return HW_TIM_STATUS_OK;
}

/* 初始化微秒计时，配置完成后等待手动开始。 */
HW_TIM_Status_e HW_TIM_TIME_US_init(TIM_TypeDef *TIMx)
{
    return HW_TIM_TimeInit(TIMx, HW_TIM_TIME_UNIT_US);
}

/* 清零并开始微秒计时。 */
HW_TIM_Status_e HW_TIM_TIME_US_Start(TIM_TypeDef *TIMx)
{
    return HW_TIM_TimeStart(TIMx, HW_TIM_TIME_UNIT_US);
}

/* 停止微秒计时并输出耗时，保留计数结果。 */
HW_TIM_Status_e HW_TIM_TIME_US_Stop(TIM_TypeDef *TIMx, uint32_t *elapsed_us)
{
    return HW_TIM_TimeStop(TIMx, HW_TIM_TIME_UNIT_US, elapsed_us);
}

/* 初始化毫秒计时，配置完成后等待手动开始。 */
HW_TIM_Status_e HW_TIM_TIME_MS_init(TIM_TypeDef *TIMx)
{
    return HW_TIM_TimeInit(TIMx, HW_TIM_TIME_UNIT_MS);
}

/* 清零并开始毫秒计时。 */
HW_TIM_Status_e HW_TIM_TIME_MS_Start(TIM_TypeDef *TIMx)
{
    return HW_TIM_TimeStart(TIMx, HW_TIM_TIME_UNIT_MS);
}

/* 停止毫秒计时并输出耗时，保留计数结果。 */
HW_TIM_Status_e HW_TIM_TIME_MS_Stop(TIM_TypeDef *TIMx, uint32_t *elapsed_ms)
{
    return HW_TIM_TimeStop(TIMx, HW_TIM_TIME_UNIT_MS, elapsed_ms);
}

/* 根据 GPIO 端口自动使能 I/O 时钟。 */
static HW_TIM_Status_e HW_TIM_EnableGPIOClock(GPIO_TypeDef *gpio_port)
{
#ifdef GPIOA
    if (gpio_port == GPIOA)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
        return HW_TIM_STATUS_OK;
    }
#endif

#ifdef GPIOB
    if (gpio_port == GPIOB)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
        return HW_TIM_STATUS_OK;
    }
#endif

#ifdef GPIOF
    if (gpio_port == GPIOF)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);
        return HW_TIM_STATUS_OK;
    }
#endif

    return HW_TIM_STATUS_UNSUPPORTED;
}

/* 将毫秒周期换算为 16 位定时器可用的 PSC 和 ARR。 */
static HW_TIM_Status_e HW_TIM_CalculatePeriod(uint32_t timer_clock_hz,
                                               uint32_t period_ms,
                                               uint16_t *out_psc,
                                               uint16_t *out_arr)
{
    uint64_t total_ticks;
    uint64_t prescaler_div;
    uint64_t autoreload_div;

    if ((timer_clock_hz == 0U) || (period_ms == 0U) ||
        (out_psc == NULL) || (out_arr == NULL))
    {
        return HW_TIM_STATUS_INVALID_ARG;
    }

    total_ticks = ((uint64_t)timer_clock_hz * (uint64_t)period_ms + 500U) / 1000U;
    if (total_ticks == 0U)
    {
        return HW_TIM_STATUS_PERIOD_OUT_OF_RANGE;
    }

    /* 先让 ARR 尽量大，以取得更细的计数分辨率。 */
    prescaler_div = (total_ticks + 65535U) / 65536U;
    if (prescaler_div > 65536U)
    {
        return HW_TIM_STATUS_PERIOD_OUT_OF_RANGE;
    }

    autoreload_div = (total_ticks + (prescaler_div / 2U)) / prescaler_div;
    if ((autoreload_div == 0U) || (autoreload_div > 65536U))
    {
        return HW_TIM_STATUS_PERIOD_OUT_OF_RANGE;
    }

    *out_psc = (uint16_t)(prescaler_div - 1U);
    *out_arr = (uint16_t)(autoreload_div - 1U);
    return HW_TIM_STATUS_OK;
}

/* 将 PWM 频率换算为 16 位定时器可用的 PSC 和 ARR。 */
static HW_TIM_Status_e HW_TIM_CalculatePWMFrequency(uint32_t timer_clock_hz,
                                                     uint32_t frequency_hz,
                                                     uint16_t *out_psc,
                                                     uint16_t *out_arr)
{
    uint64_t total_ticks;
    uint64_t prescaler_div;
    uint64_t autoreload_div;

    if ((timer_clock_hz == 0U) || (frequency_hz == 0U) ||
        (out_psc == NULL) || (out_arr == NULL))
    {
        return HW_TIM_STATUS_INVALID_ARG;
    }

    if (frequency_hz > timer_clock_hz)
    {
        return HW_TIM_STATUS_FREQUENCY_OUT_OF_RANGE;
    }

    total_ticks = ((uint64_t)timer_clock_hz + ((uint64_t)frequency_hz / 2U)) /
                  (uint64_t)frequency_hz;
    if (total_ticks == 0U)
    {
        return HW_TIM_STATUS_FREQUENCY_OUT_OF_RANGE;
    }

    prescaler_div = (total_ticks + 65535U) / 65536U;
    if (prescaler_div > 65536U)
    {
        return HW_TIM_STATUS_FREQUENCY_OUT_OF_RANGE;
    }

    autoreload_div = (total_ticks + (prescaler_div / 2U)) / prescaler_div;
    if ((autoreload_div == 0U) || (autoreload_div > 65536U))
    {
        return HW_TIM_STATUS_FREQUENCY_OUT_OF_RANGE;
    }

    *out_psc = (uint16_t)(prescaler_div - 1U);
    *out_arr = (uint16_t)(autoreload_div - 1U);
    return HW_TIM_STATUS_OK;
}

/* 基础定时器更新中断初始化：TIMx、UP/DOWN、周期（ms）。 */
HW_TIM_Status_e HW_TIM_COUNT_IT_init(TIM_TypeDef *TIMx,
                                     uint8_t count_mode,
                                     uint32_t period_ms)
{
    LL_TIM_InitTypeDef tim_init = {0};
    LL_RCC_ClocksTypeDef rcc_clocks;
    IRQn_Type update_irq;
    HW_TIM_Status_e status;
    uint16_t psc;
    uint16_t arr;

    if ((count_mode != UP) && (count_mode != DOWN))
    {
        return HW_TIM_STATUS_INVALID_ARG;
    }

    if ((HW_TIM_TimeState(TIMx) != NULL) &&
        (HW_TIM_TimeState(TIMx)->initialized != 0U))
        return HW_TIM_STATUS_BUSY;

    status = HW_TIM_EnableClockAndGetIRQ(TIMx, &update_irq);
    if (status != HW_TIM_STATUS_OK)
    {
        return status;
    }

    /* 当前 system_init 固定 APB1 = HCLK；因此 PCLK1 就是本项目的 TIM 时钟。 */
    LL_RCC_GetSystemClocksFreq(&rcc_clocks);
    status = HW_TIM_CalculatePeriod(rcc_clocks.PCLK1_Frequency,
                                    period_ms,
                                    &psc,
                                    &arr);
    if (status != HW_TIM_STATUS_OK)
    {
        return status;
    }

    LL_TIM_DisableCounter(TIMx);
    LL_TIM_DisableIT_UPDATE(TIMx);
    LL_TIM_ClearFlag_UPDATE(TIMx);

    tim_init.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    tim_init.CounterMode = (count_mode == UP) ? LL_TIM_COUNTERMODE_UP : LL_TIM_COUNTERMODE_DOWN;
    tim_init.Prescaler = psc;
    tim_init.Autoreload = arr;
    tim_init.RepetitionCounter = 0U;

    if (LL_TIM_Init(TIMx, &tim_init) != SUCCESS)
    {
        return HW_TIM_STATUS_INIT_FAILED;
    }

    /* 强制装载 PSC，并清除这次装载产生的更新标志，避免一开启就误进中断。 */
    LL_TIM_GenerateEvent_UPDATE(TIMx);
    LL_TIM_ClearFlag_UPDATE(TIMx);

    NVIC_SetPriority(update_irq, 0U);
    NVIC_EnableIRQ(update_irq);
    LL_TIM_EnableIT_UPDATE(TIMx);
    LL_TIM_EnableCounter(TIMx);

    return HW_TIM_STATUS_OK;
}

/* PWM 初始化：TIMx、PWM 通道/引脚、频率（Hz）、占空比（0~100）。 */
HW_TIM_Status_e HW_TIM_PWM_init(TIM_TypeDef *TIMx,
                                TIM_PWM_Channel_e channel,
                                uint32_t frequency_hz,
                                uint8_t duty_percent)
{
    const HW_TIM_PWM_Map_t *pwm_cfg;
    LL_GPIO_InitTypeDef gpio_init = {0};
    LL_TIM_InitTypeDef tim_init = {0};
    LL_TIM_OC_InitTypeDef oc_init = {0};
    LL_RCC_ClocksTypeDef rcc_clocks;
    HW_TIM_Status_e status;
    uint16_t psc;
    uint16_t arr;
    uint32_t timer_counts;

    if (((uint32_t)channel >= (uint32_t)PWM_CHANNEL_COUNT) || (duty_percent > 100U))
    {
        return HW_TIM_STATUS_INVALID_ARG;
    }

    if ((HW_TIM_TimeState(TIMx) != NULL) &&
        (HW_TIM_TimeState(TIMx)->initialized != 0U))
        return HW_TIM_STATUS_BUSY;

    pwm_cfg = &HW_TIM_PWM_MAP[channel];
    if ((TIMx == NULL) || (pwm_cfg->tim_instance != TIMx) ||
        (pwm_cfg->gpio_port == NULL) || (pwm_cfg->gpio_pin == 0U))
    {
        return HW_TIM_STATUS_INVALID_ARG;
    }

    status = HW_TIM_EnableClockAndGetIRQ(TIMx, NULL);
    if (status != HW_TIM_STATUS_OK)
    {
        return status;
    }

    status = HW_TIM_EnableGPIOClock(pwm_cfg->gpio_port);
    if (status != HW_TIM_STATUS_OK)
    {
        return status;
    }

    gpio_init.Pin = pwm_cfg->gpio_pin;
    gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed = HW_TIM_PWM_GPIO_SPEED;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = LL_GPIO_PULL_NO;
    gpio_init.Alternate = pwm_cfg->gpio_alternate;
    if (LL_GPIO_Init(pwm_cfg->gpio_port, &gpio_init) != SUCCESS)
    {
        return HW_TIM_STATUS_GPIO_INIT_FAILED;
    }

    /* 当前 system_init 固定 APB1 = HCLK；因此 PCLK1 就是本项目的 TIM 时钟。 */
    LL_RCC_GetSystemClocksFreq(&rcc_clocks);
    status = HW_TIM_CalculatePWMFrequency(rcc_clocks.PCLK1_Frequency,
                                           frequency_hz,
                                           &psc,
                                           &arr);
    if (status != HW_TIM_STATUS_OK)
    {
        return status;
    }

    LL_TIM_DisableCounter(TIMx);
    tim_init.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    tim_init.CounterMode = LL_TIM_COUNTERMODE_UP;
    tim_init.Prescaler = psc;
    tim_init.Autoreload = arr;
    tim_init.RepetitionCounter = 0U;
    if (LL_TIM_Init(TIMx, &tim_init) != SUCCESS)
    {
        return HW_TIM_STATUS_INIT_FAILED;
    }

    timer_counts = (uint32_t)arr + 1U;
    oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
    oc_init.OCPolarity = HW_TIM_PWM_OC_POLARITY;
    oc_init.OCIdleState = HW_TIM_PWM_OC_IDLE_STATE;

    if (duty_percent == 0U)
    {
        oc_init.OCMode = LL_TIM_OCMODE_FORCED_INACTIVE;
        oc_init.CompareValue = 0U;
    }
    else if (duty_percent == 100U)
    {
        oc_init.OCMode = LL_TIM_OCMODE_FORCED_ACTIVE;
        oc_init.CompareValue = 0U;
    }
    else
    {
        oc_init.OCMode = HW_TIM_PWM_OC_MODE;
        oc_init.CompareValue = (timer_counts * (uint32_t)duty_percent) / 100U;
    }

    if (LL_TIM_OC_Init(TIMx, pwm_cfg->tim_channel, &oc_init) != SUCCESS)
    {
        return HW_TIM_STATUS_INIT_FAILED;
    }

    LL_TIM_GenerateEvent_UPDATE(TIMx);
    if (pwm_cfg->main_output_enable != 0U)
    {
        LL_TIM_EnableAllOutputs(TIMx);
    }
    LL_TIM_EnableCounter(TIMx);

    return HW_TIM_STATUS_OK;
}

/* 各定时器 IRQ 只处理自身的更新标志，清标志后再交给对应的 weak 回调。 */
#ifdef TIM1
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM1) && LL_TIM_IsEnabledIT_UPDATE(TIM1))
    {
        LL_TIM_ClearFlag_UPDATE(TIM1);
        TIM1_UpdateCallback();
    }
}
#endif

#ifdef TIM3
void TIM3_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM3) && LL_TIM_IsEnabledIT_UPDATE(TIM3))
    {
        LL_TIM_ClearFlag_UPDATE(TIM3);
        TIM3_UpdateCallback();
    }
}
#endif

#ifdef TIM14
void TIM14_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM14) && LL_TIM_IsEnabledIT_UPDATE(TIM14))
    {
        LL_TIM_ClearFlag_UPDATE(TIM14);
        TIM14_UpdateCallback();
    }
}
#endif

#ifdef TIM16
void TIM16_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM16) && LL_TIM_IsEnabledIT_UPDATE(TIM16))
    {
        LL_TIM_ClearFlag_UPDATE(TIM16);
        TIM16_UpdateCallback();
    }
}
#endif

#ifdef TIM17
void TIM17_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM17) && LL_TIM_IsEnabledIT_UPDATE(TIM17))
    {
        LL_TIM_ClearFlag_UPDATE(TIM17);
        TIM17_UpdateCallback();
    }
}
#endif

/*中断返回*/
__weak void TIM1_UpdateCallback(void){}
__weak void TIM3_UpdateCallback(void){}
__weak void TIM14_UpdateCallback(void){}
__weak void TIM16_UpdateCallback(void){}
__weak void TIM17_UpdateCallback(void){}


