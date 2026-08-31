#include "./HW_INIT/LOWPOWER/LowPower.h"

#include "./SYS/system_init.h"
#include "py32f0xx_ll_adc.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_cortex.h"
#include "py32f0xx_ll_dma.h"
#include "py32f0xx_ll_i2c.h"
#include "py32f0xx_ll_pwr.h"
#include "py32f0xx_ll_spi.h"
#include "py32f0xx_ll_tim.h"
#include "py32f0xx_ll_usart.h"

#define LOWPOWER_IRQ_UNUSED                 (-1)

typedef enum
{
    LOWPOWER_TIMER_BUS_APB1_GROUP1,
    LOWPOWER_TIMER_BUS_APB1_GROUP2,
} LowPower_TimerBus_e;

typedef struct
{
    IRQn_Type irqn;
    uint32_t was_enabled;
} LowPower_IrqState_t;

typedef struct
{
    uint32_t clock_was_enabled;
    uint32_t cr1;
    uint32_t dier;
    uint32_t ccer;
    uint32_t bdtr;
    LowPower_IrqState_t irq1;
    LowPower_IrqState_t irq2;
} LowPower_TimerState_t;

typedef struct
{
    uint32_t ahb1_clock_mask;
    uint32_t apb1_group1_clock_mask;
    uint32_t apb1_group2_clock_mask;
    uint32_t spi1_cr1;
    uint32_t spi1_cr2;
    uint32_t i2c1_cr1;
    uint32_t i2c1_cr2;
    uint32_t usart1_cr1;
    uint32_t usart1_cr3;
    uint32_t usart2_cr1;
    uint32_t usart2_cr3;
    uint32_t adc1_was_prepared;
    uint32_t adc1_was_enabled;
    uint32_t adc1_ier;
    uint32_t dma1_channel1_ccr;
    uint32_t dma1_channel2_ccr;
    uint32_t dma1_channel3_ccr;
    LowPower_IrqState_t spi1_irq;
    LowPower_IrqState_t i2c1_irq;
    LowPower_IrqState_t usart1_irq;
    LowPower_IrqState_t usart2_irq;
    LowPower_IrqState_t dma1_channel1_irq;
    LowPower_IrqState_t dma1_channel2_3_irq;
} LowPower_PeripheralState_t;

static LowPower_PeripheralState_t s_peripheral_state;

#if LOWPOWER_MANAGE_TIM1 && defined(TIM1)
static LowPower_TimerState_t s_tim1_state;
#endif
#if LOWPOWER_MANAGE_TIM3 && defined(TIM3)
static LowPower_TimerState_t s_tim3_state;
#endif
#if LOWPOWER_MANAGE_TIM14 && defined(TIM14)
static LowPower_TimerState_t s_tim14_state;
#endif
#if LOWPOWER_MANAGE_TIM16 && defined(TIM16)
static LowPower_TimerState_t s_tim16_state;
#endif
#if LOWPOWER_MANAGE_TIM17 && defined(TIM17)
static LowPower_TimerState_t s_tim17_state;
#endif

/* 读取指定NVIC中断在休眠前是否处于使能状态。 */
static uint32_t lowpower_irq_is_enabled(IRQn_Type irqn)
{
    return ((NVIC->ISER[0] & (1UL << ((uint32_t)irqn & 0x1FU))) != 0U) ? 1U : 0U;
}

/* 保存并关闭一个NVIC中断，同时丢弃休眠前尚未处理的旧Pending。 */
static void lowpower_suspend_irq(LowPower_IrqState_t *p_state, IRQn_Type irqn)
{
    p_state->irqn = irqn;
    p_state->was_enabled = lowpower_irq_is_enabled(irqn);
    NVIC_DisableIRQ(irqn);
    NVIC_ClearPendingIRQ(irqn);
}

/* 按休眠前状态恢复一个NVIC中断。 */
static void lowpower_resume_irq(const LowPower_IrqState_t *p_state)
{
    NVIC_ClearPendingIRQ(p_state->irqn);
    if (p_state->was_enabled != 0U)
    {
        NVIC_EnableIRQ(p_state->irqn);
    }
}

/* 查询指定定时器的APB时钟是否开启。 */
static uint32_t lowpower_timer_clock_is_enabled(LowPower_TimerBus_e bus, uint32_t clock_mask)
{
    if (bus == LOWPOWER_TIMER_BUS_APB1_GROUP1)
    {
        return LL_APB1_GRP1_IsEnabledClock(clock_mask);
    }

    return LL_APB1_GRP2_IsEnabledClock(clock_mask);
}

/* 关闭指定定时器的APB时钟。 */
static void lowpower_timer_clock_disable(LowPower_TimerBus_e bus, uint32_t clock_mask)
{
    if (bus == LOWPOWER_TIMER_BUS_APB1_GROUP1)
    {
        LL_APB1_GRP1_DisableClock(clock_mask);
    }
    else
    {
        LL_APB1_GRP2_DisableClock(clock_mask);
    }
}

/* 开启指定定时器的APB时钟。 */
static void lowpower_timer_clock_enable(LowPower_TimerBus_e bus, uint32_t clock_mask)
{
    if (bus == LOWPOWER_TIMER_BUS_APB1_GROUP1)
    {
        LL_APB1_GRP1_EnableClock(clock_mask);
    }
    else
    {
        LL_APB1_GRP2_EnableClock(clock_mask);
    }
}

/* 保存一个定时器的运行状态并停止其计数、请求和时钟。 */
static void lowpower_suspend_timer(TIM_TypeDef *p_timer,
                                   LowPower_TimerBus_e bus,
                                   uint32_t clock_mask,
                                   IRQn_Type irq1,
                                   int32_t irq2,
                                   uint32_t has_main_output,
                                   LowPower_TimerState_t *p_state)
{
    p_state->clock_was_enabled = lowpower_timer_clock_is_enabled(bus, clock_mask);
    lowpower_suspend_irq(&p_state->irq1, irq1);

    if (irq2 >= 0)
    {
        lowpower_suspend_irq(&p_state->irq2, (IRQn_Type)irq2);
    }

    if (p_state->clock_was_enabled == 0U)
    {
        return;
    }

    p_state->cr1 = LL_TIM_ReadReg(p_timer, CR1);
    p_state->dier = LL_TIM_ReadReg(p_timer, DIER);
    p_state->ccer = LL_TIM_ReadReg(p_timer, CCER);
    if (has_main_output != 0U)
    {
        p_state->bdtr = LL_TIM_ReadReg(p_timer, BDTR);
    }

    LL_TIM_WriteReg(p_timer, DIER, 0U);
    LL_TIM_DisableCounter(p_timer);
    LL_TIM_WriteReg(p_timer, CCER, 0U);
    if (has_main_output != 0U)
    {
        CLEAR_BIT(p_timer->BDTR, TIM_BDTR_MOE);
    }
    LL_TIM_WriteReg(p_timer, SR, 0U);
    lowpower_timer_clock_disable(bus, clock_mask);
}

/* 按休眠前状态恢复一个定时器的时钟、请求、中断和计数。 */
static void lowpower_resume_timer(TIM_TypeDef *p_timer,
                                  LowPower_TimerBus_e bus,
                                  uint32_t clock_mask,
                                  int32_t irq2,
                                  uint32_t has_main_output,
                                  const LowPower_TimerState_t *p_state)
{
    if (p_state->clock_was_enabled != 0U)
    {
        lowpower_timer_clock_enable(bus, clock_mask);
        LL_TIM_WriteReg(p_timer, DIER, 0U);
        LL_TIM_DisableCounter(p_timer);
        LL_TIM_WriteReg(p_timer, SR, 0U);
        LL_TIM_WriteReg(p_timer, CCER, p_state->ccer);
        if (has_main_output != 0U)
        {
            LL_TIM_WriteReg(p_timer, BDTR, p_state->bdtr);
        }
        LL_TIM_WriteReg(p_timer, DIER, p_state->dier);
    }

    lowpower_resume_irq(&p_state->irq1);

    if (irq2 >= 0)
    {
        lowpower_resume_irq(&p_state->irq2);
    }

    if (p_state->clock_was_enabled != 0U)
    {
        LL_TIM_WriteReg(p_timer, CR1, p_state->cr1);
    }
}

/* 按休眠前状态重新开启ADC并恢复其中断配置。 */
static void lowpower_restore_adc(void)
{
#if LOWPOWER_MANAGE_ADC1 && defined(ADC1)
    uint32_t delay_cycles;

    if (s_peripheral_state.adc1_was_prepared == 0U)
    {
        return;
    }

    if (LL_APB1_GRP2_IsEnabledClock(LL_APB1_GRP2_PERIPH_ADC1) == 0U)
    {
        LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_ADC1);
    }
    if ((s_peripheral_state.adc1_was_enabled != 0U) &&
        (LL_ADC_IsEnabled(ADC1) == 0U))
    {
        LL_ADC_Enable(ADC1);
        for (delay_cycles = 0U;
             delay_cycles < LL_ADC_DELAY_CALIB_ENABLE_CPU_CYCLES;
             delay_cycles++)
        {
            __NOP();
        }
    }

    ADC1->IER = s_peripheral_state.adc1_ier;
    s_peripheral_state.adc1_was_prepared = 0U;
#endif
}

/* 在关闭全局中断前安全停止ADC，避开LL_ADC_Disable内部的中断切换。 */
static uint32_t lowpower_prepare_adc(void)
{
#if LOWPOWER_MANAGE_ADC1 && defined(ADC1)
    s_peripheral_state.adc1_was_prepared = 0U;
    if (LL_APB1_GRP2_IsEnabledClock(LL_APB1_GRP2_PERIPH_ADC1) == 0U)
    {
        return 1U;
    }
    if (LL_ADC_REG_IsConversionOngoing(ADC1) != 0U)
    {
        return 0U;
    }

    s_peripheral_state.adc1_was_enabled = LL_ADC_IsEnabled(ADC1);
    s_peripheral_state.adc1_ier = ADC1->IER;
    s_peripheral_state.adc1_was_prepared = 1U;
    ADC1->IER = 0U;

    if ((s_peripheral_state.adc1_was_enabled != 0U) &&
        (LL_ADC_Disable(ADC1) != SUCCESS))
    {
        lowpower_restore_adc();
        return 0U;
    }
#endif

    return 1U;
}

/* 检查通信、转换和DMA是否已经完成，忙时禁止直接进入Stop。 */
static uint32_t lowpower_peripheral_is_busy(void)
{
#if LOWPOWER_MANAGE_ADC1 && defined(ADC1)
    if ((LL_APB1_GRP2_IsEnabledClock(LL_APB1_GRP2_PERIPH_ADC1) != 0U) &&
        ((LL_ADC_REG_IsConversionOngoing(ADC1) != 0U) ||
         ((s_peripheral_state.adc1_was_prepared != 0U) && (LL_ADC_IsEnabled(ADC1) != 0U))))
    {
        return 1U;
    }
#endif

#if LOWPOWER_MANAGE_SPI1 && defined(SPI1)
    if ((LL_APB1_GRP2_IsEnabledClock(LL_APB1_GRP2_PERIPH_SPI1) != 0U) &&
        ((LL_SPI_IsActiveFlag_BSY(SPI1) != 0U) ||
         (LL_SPI_GetTxFIFOLevel(SPI1) != LL_SPI_TX_FIFO_EMPTY) ||
         (LL_SPI_GetRxFIFOLevel(SPI1) != LL_SPI_RX_FIFO_EMPTY)))
    {
        return 1U;
    }
#if defined(DMA1)
    if ((LL_APB1_GRP2_IsEnabledClock(LL_APB1_GRP2_PERIPH_SPI1) != 0U) &&
        ((READ_BIT(SPI1->CR2, SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN)) != 0U) &&
        (LL_AHB1_GRP1_IsEnabledClock(LL_AHB1_GRP1_PERIPH_DMA1) != 0U) &&
        ((LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_1) != 0U) ||
         (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_2) != 0U)))
    {
        return 1U;
    }
#endif
#endif

#if LOWPOWER_MANAGE_I2C1 && defined(I2C1)
    if ((LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_I2C1) != 0U) &&
        (LL_I2C_IsEnabled(I2C1) != 0U) &&
        (LL_I2C_IsActiveFlag_BUSY(I2C1) != 0U))
    {
        return 1U;
    }
#endif

#if LOWPOWER_MANAGE_USART1 && defined(USART1)
    if ((LL_APB1_GRP2_IsEnabledClock(LL_APB1_GRP2_PERIPH_USART1) != 0U) &&
        (LL_USART_IsEnabled(USART1) != 0U) &&
        ((((READ_BIT(USART1->CR1, USART_CR1_TE)) != 0U) &&
          (LL_USART_IsActiveFlag_TC(USART1) == 0U)) ||
         (LL_USART_IsActiveFlag_RXNE(USART1) != 0U) ||
         (LL_USART_IsActiveFlag_ORE(USART1) != 0U) ||
         (LL_USART_IsActiveFlag_FE(USART1) != 0U) ||
         (LL_USART_IsActiveFlag_NE(USART1) != 0U)))
    {
        return 1U;
    }
#endif

#if LOWPOWER_MANAGE_USART2 && defined(USART2)
    if ((LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_USART2) != 0U) &&
        (LL_USART_IsEnabled(USART2) != 0U) &&
        ((((READ_BIT(USART2->CR1, USART_CR1_TE)) != 0U) &&
          (LL_USART_IsActiveFlag_TC(USART2) == 0U)) ||
         (LL_USART_IsActiveFlag_RXNE(USART2) != 0U) ||
         (LL_USART_IsActiveFlag_ORE(USART2) != 0U) ||
         (LL_USART_IsActiveFlag_FE(USART2) != 0U) ||
         (LL_USART_IsActiveFlag_NE(USART2) != 0U)))
    {
        return 1U;
    }
#endif

#if LOWPOWER_MANAGE_DMA1 && defined(DMA1)
    if ((LL_AHB1_GRP1_IsEnabledClock(LL_AHB1_GRP1_PERIPH_DMA1) != 0U) &&
        ((LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_1) != 0U) ||
         (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_2) != 0U) ||
         (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_3) != 0U)))
    {
        return 1U;
    }
#endif

    return 0U;
}

/* 保存并暂停由配置开关选中的普通外设。 */
static void lowpower_suspend_peripherals(void)
{
    s_peripheral_state.ahb1_clock_mask = 0U;
    s_peripheral_state.apb1_group1_clock_mask = 0U;
    s_peripheral_state.apb1_group2_clock_mask = 0U;

#if LOWPOWER_MANAGE_DMA1 && defined(DMA1)
    lowpower_suspend_irq(&s_peripheral_state.dma1_channel1_irq, DMA1_Channel1_IRQn);
    lowpower_suspend_irq(&s_peripheral_state.dma1_channel2_3_irq, DMA1_Channel2_3_IRQn);
    if (LL_AHB1_GRP1_IsEnabledClock(LL_AHB1_GRP1_PERIPH_DMA1) != 0U)
    {
        s_peripheral_state.ahb1_clock_mask |= LL_AHB1_GRP1_PERIPH_DMA1;
        s_peripheral_state.dma1_channel1_ccr = DMA1_Channel1->CCR;
        s_peripheral_state.dma1_channel2_ccr = DMA1_Channel2->CCR;
        s_peripheral_state.dma1_channel3_ccr = DMA1_Channel3->CCR;
        CLEAR_BIT(DMA1_Channel1->CCR, DMA_CCR_TCIE | DMA_CCR_HTIE | DMA_CCR_TEIE);
        CLEAR_BIT(DMA1_Channel2->CCR, DMA_CCR_TCIE | DMA_CCR_HTIE | DMA_CCR_TEIE);
        CLEAR_BIT(DMA1_Channel3->CCR, DMA_CCR_TCIE | DMA_CCR_HTIE | DMA_CCR_TEIE);
        LL_DMA_ClearFlag_GI1(DMA1);
        LL_DMA_ClearFlag_GI2(DMA1);
        LL_DMA_ClearFlag_GI3(DMA1);
    }
#endif

#if LOWPOWER_MANAGE_I2C1 && defined(I2C1)
    lowpower_suspend_irq(&s_peripheral_state.i2c1_irq, I2C1_IRQn);
    if (LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_I2C1) != 0U)
    {
        s_peripheral_state.apb1_group1_clock_mask |= LL_APB1_GRP1_PERIPH_I2C1;
        s_peripheral_state.i2c1_cr1 = I2C1->CR1;
        s_peripheral_state.i2c1_cr2 = I2C1->CR2;
        CLEAR_BIT(I2C1->CR2, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN | I2C_CR2_DMAEN);
        LL_I2C_Disable(I2C1);
    }
#endif

#if LOWPOWER_MANAGE_USART2 && defined(USART2)
    lowpower_suspend_irq(&s_peripheral_state.usart2_irq, USART2_IRQn);
    if (LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_USART2) != 0U)
    {
        s_peripheral_state.apb1_group1_clock_mask |= LL_APB1_GRP1_PERIPH_USART2;
        s_peripheral_state.usart2_cr1 = USART2->CR1;
        s_peripheral_state.usart2_cr3 = USART2->CR3;
        CLEAR_BIT(USART2->CR3, USART_CR3_EIE | USART_CR3_CTSIE | USART_CR3_DMAR | USART_CR3_DMAT);
        CLEAR_BIT(USART2->CR1, USART_CR1_IDLEIE | USART_CR1_RXNEIE | USART_CR1_TCIE |
                                  USART_CR1_TXEIE | USART_CR1_PEIE);
        LL_USART_Disable(USART2);
    }
#endif

#if LOWPOWER_MANAGE_ADC1 && defined(ADC1)
    if (LL_APB1_GRP2_IsEnabledClock(LL_APB1_GRP2_PERIPH_ADC1) != 0U)
    {
        s_peripheral_state.apb1_group2_clock_mask |= LL_APB1_GRP2_PERIPH_ADC1;
    }
#endif

#if LOWPOWER_MANAGE_SPI1 && defined(SPI1)
    lowpower_suspend_irq(&s_peripheral_state.spi1_irq, SPI1_IRQn);
    if (LL_APB1_GRP2_IsEnabledClock(LL_APB1_GRP2_PERIPH_SPI1) != 0U)
    {
        s_peripheral_state.apb1_group2_clock_mask |= LL_APB1_GRP2_PERIPH_SPI1;
        s_peripheral_state.spi1_cr1 = SPI1->CR1;
        s_peripheral_state.spi1_cr2 = SPI1->CR2;
        CLEAR_BIT(SPI1->CR2, SPI_CR2_ERRIE | SPI_CR2_RXNEIE | SPI_CR2_TXEIE |
                                SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
        LL_SPI_Disable(SPI1);
    }
#endif

#if LOWPOWER_MANAGE_USART1 && defined(USART1)
    lowpower_suspend_irq(&s_peripheral_state.usart1_irq, USART1_IRQn);
    if (LL_APB1_GRP2_IsEnabledClock(LL_APB1_GRP2_PERIPH_USART1) != 0U)
    {
        s_peripheral_state.apb1_group2_clock_mask |= LL_APB1_GRP2_PERIPH_USART1;
        s_peripheral_state.usart1_cr1 = USART1->CR1;
        s_peripheral_state.usart1_cr3 = USART1->CR3;
        CLEAR_BIT(USART1->CR3, USART_CR3_EIE | USART_CR3_CTSIE | USART_CR3_DMAR | USART_CR3_DMAT);
        CLEAR_BIT(USART1->CR1, USART_CR1_IDLEIE | USART_CR1_RXNEIE | USART_CR1_TCIE |
                                  USART_CR1_TXEIE | USART_CR1_PEIE);
        LL_USART_Disable(USART1);
    }
#endif

    if (s_peripheral_state.ahb1_clock_mask != 0U)
    {
        LL_AHB1_GRP1_DisableClock(s_peripheral_state.ahb1_clock_mask);
    }
    if (s_peripheral_state.apb1_group1_clock_mask != 0U)
    {
        LL_APB1_GRP1_DisableClock(s_peripheral_state.apb1_group1_clock_mask);
    }
    if (s_peripheral_state.apb1_group2_clock_mask != 0U)
    {
        LL_APB1_GRP2_DisableClock(s_peripheral_state.apb1_group2_clock_mask);
    }
}

/* 按休眠前状态恢复由配置开关选中的普通外设。 */
static void lowpower_resume_peripherals(void)
{
    if (s_peripheral_state.ahb1_clock_mask != 0U)
    {
        LL_AHB1_GRP1_EnableClock(s_peripheral_state.ahb1_clock_mask);
    }
    if (s_peripheral_state.apb1_group1_clock_mask != 0U)
    {
        LL_APB1_GRP1_EnableClock(s_peripheral_state.apb1_group1_clock_mask);
    }
    if (s_peripheral_state.apb1_group2_clock_mask != 0U)
    {
        LL_APB1_GRP2_EnableClock(s_peripheral_state.apb1_group2_clock_mask);
    }

#if LOWPOWER_MANAGE_DMA1 && defined(DMA1)
    if ((s_peripheral_state.ahb1_clock_mask & LL_AHB1_GRP1_PERIPH_DMA1) != 0U)
    {
        DMA1_Channel1->CCR = s_peripheral_state.dma1_channel1_ccr;
        DMA1_Channel2->CCR = s_peripheral_state.dma1_channel2_ccr;
        DMA1_Channel3->CCR = s_peripheral_state.dma1_channel3_ccr;
    }
    lowpower_resume_irq(&s_peripheral_state.dma1_channel1_irq);
    lowpower_resume_irq(&s_peripheral_state.dma1_channel2_3_irq);
#endif

#if LOWPOWER_MANAGE_I2C1 && defined(I2C1)
    if ((s_peripheral_state.apb1_group1_clock_mask & LL_APB1_GRP1_PERIPH_I2C1) != 0U)
    {
        I2C1->CR2 = s_peripheral_state.i2c1_cr2;
        I2C1->CR1 = s_peripheral_state.i2c1_cr1;
    }
    lowpower_resume_irq(&s_peripheral_state.i2c1_irq);
#endif

#if LOWPOWER_MANAGE_USART2 && defined(USART2)
    if ((s_peripheral_state.apb1_group1_clock_mask & LL_APB1_GRP1_PERIPH_USART2) != 0U)
    {
        USART2->CR3 = s_peripheral_state.usart2_cr3;
        USART2->CR1 = s_peripheral_state.usart2_cr1;
    }
    lowpower_resume_irq(&s_peripheral_state.usart2_irq);
#endif

#if LOWPOWER_MANAGE_ADC1 && defined(ADC1)
    lowpower_restore_adc();
#endif

#if LOWPOWER_MANAGE_SPI1 && defined(SPI1)
    if ((s_peripheral_state.apb1_group2_clock_mask & LL_APB1_GRP2_PERIPH_SPI1) != 0U)
    {
        SPI1->CR2 = s_peripheral_state.spi1_cr2;
        SPI1->CR1 = s_peripheral_state.spi1_cr1;
    }
    lowpower_resume_irq(&s_peripheral_state.spi1_irq);
#endif

#if LOWPOWER_MANAGE_USART1 && defined(USART1)
    if ((s_peripheral_state.apb1_group2_clock_mask & LL_APB1_GRP2_PERIPH_USART1) != 0U)
    {
        USART1->CR3 = s_peripheral_state.usart1_cr3;
        USART1->CR1 = s_peripheral_state.usart1_cr1;
    }
    lowpower_resume_irq(&s_peripheral_state.usart1_irq);
#endif
}

/* 保存并暂停由配置开关选中的全部普通定时器。 */
static void lowpower_suspend_timers(void)
{
#if LOWPOWER_MANAGE_TIM1 && defined(TIM1)
    lowpower_suspend_timer(TIM1, LOWPOWER_TIMER_BUS_APB1_GROUP2,
                           LL_APB1_GRP2_PERIPH_TIM1,
                           TIM1_BRK_UP_TRG_COM_IRQn, TIM1_CC_IRQn, 1U, &s_tim1_state);
#endif
#if LOWPOWER_MANAGE_TIM3 && defined(TIM3)
    lowpower_suspend_timer(TIM3, LOWPOWER_TIMER_BUS_APB1_GROUP1,
                           LL_APB1_GRP1_PERIPH_TIM3,
                           TIM3_IRQn, LOWPOWER_IRQ_UNUSED, 0U, &s_tim3_state);
#endif
#if LOWPOWER_MANAGE_TIM14 && defined(TIM14)
    lowpower_suspend_timer(TIM14, LOWPOWER_TIMER_BUS_APB1_GROUP2,
                           LL_APB1_GRP2_PERIPH_TIM14,
                           TIM14_IRQn, LOWPOWER_IRQ_UNUSED, 0U, &s_tim14_state);
#endif
#if LOWPOWER_MANAGE_TIM16 && defined(TIM16)
    lowpower_suspend_timer(TIM16, LOWPOWER_TIMER_BUS_APB1_GROUP2,
                           LL_APB1_GRP2_PERIPH_TIM16,
                           TIM16_IRQn, LOWPOWER_IRQ_UNUSED, 1U, &s_tim16_state);
#endif
#if LOWPOWER_MANAGE_TIM17 && defined(TIM17)
    lowpower_suspend_timer(TIM17, LOWPOWER_TIMER_BUS_APB1_GROUP2,
                           LL_APB1_GRP2_PERIPH_TIM17,
                           TIM17_IRQn, LOWPOWER_IRQ_UNUSED, 1U, &s_tim17_state);
#endif
}

/* 按休眠前状态恢复由配置开关选中的全部普通定时器。 */
static void lowpower_resume_timers(void)
{
#if LOWPOWER_MANAGE_TIM1 && defined(TIM1)
    lowpower_resume_timer(TIM1, LOWPOWER_TIMER_BUS_APB1_GROUP2,
                          LL_APB1_GRP2_PERIPH_TIM1,
                          TIM1_CC_IRQn, 1U, &s_tim1_state);
#endif
#if LOWPOWER_MANAGE_TIM3 && defined(TIM3)
    lowpower_resume_timer(TIM3, LOWPOWER_TIMER_BUS_APB1_GROUP1,
                          LL_APB1_GRP1_PERIPH_TIM3,
                          LOWPOWER_IRQ_UNUSED, 0U, &s_tim3_state);
#endif
#if LOWPOWER_MANAGE_TIM14 && defined(TIM14)
    lowpower_resume_timer(TIM14, LOWPOWER_TIMER_BUS_APB1_GROUP2,
                          LL_APB1_GRP2_PERIPH_TIM14,
                          LOWPOWER_IRQ_UNUSED, 0U, &s_tim14_state);
#endif
#if LOWPOWER_MANAGE_TIM16 && defined(TIM16)
    lowpower_resume_timer(TIM16, LOWPOWER_TIMER_BUS_APB1_GROUP2,
                          LL_APB1_GRP2_PERIPH_TIM16,
                          LOWPOWER_IRQ_UNUSED, 1U, &s_tim16_state);
#endif
#if LOWPOWER_MANAGE_TIM17 && defined(TIM17)
    lowpower_resume_timer(TIM17, LOWPOWER_TIMER_BUS_APB1_GROUP2,
                          LL_APB1_GRP2_PERIPH_TIM17,
                          LOWPOWER_IRQ_UNUSED, 1U, &s_tim17_state);
#endif
}

/* 根据头文件配置选择Stop0或Stop1的稳压器和保持电压。 */
static void lowpower_configure_stop_mode(void)
{
#if (LOWPOWER_STOP_MODE == LOWPOWER_STOP_MODE_STOP0)
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_PWR_DisableLowPowerRunMode();
#elif (LOWPOWER_STOP_MODE == LOWPOWER_STOP_MODE_STOP1_1V2)
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_PWR_EnableLowPowerRunMode();
#else
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
    LL_PWR_EnableLowPowerRunMode();
#endif
}

/* 默认不处理板级负载，用户可在APP层用同名函数覆盖。 */
__weak uint8_t HW_LowPower_PrepareCallback(void)
{
    return 1U;
}

/* 默认不恢复板级负载，用户可在APP层用同名函数覆盖。 */
__weak void HW_LowPower_RestoreCallback(void)
{
}

/* 自动暂停已选择的外设并进入Stop，唤醒且恢复完成后才返回。 */
HW_LowPower_Status_e HW_LowPower_Enter(void)
{
    HW_GPIO_Status_e gpio_status;
    uint32_t was_pwr_clock_enabled;

    /* 第一步：检查调用环境并让板级负载完成休眠准备。 */
    if (__get_IPSR() != 0U)
    {
        return HW_LOWPOWER_STATUS_HANDLER_MODE;
    }
    if (__get_PRIMASK() != 0U)
    {
        return HW_LOWPOWER_STATUS_INTERRUPTS_DISABLED;
    }
    if (HW_LowPower_PrepareCallback() == 0U)
    {
        return HW_LOWPOWER_STATUS_PERIPHERAL_BUSY;
    }
    if (lowpower_prepare_adc() == 0U)
    {
        HW_LowPower_RestoreCallback();
        return HW_LOWPOWER_STATUS_SUSPEND_FAILED;
    }

    /* 第二步：锁住进入窗口并确认外设空闲，避免检查后又启动新的传输。 */
    __disable_irq();
    if (lowpower_peripheral_is_busy() != 0U)
    {
        lowpower_restore_adc();
        HW_LowPower_RestoreCallback();
        __enable_irq();
        return HW_LOWPOWER_STATUS_PERIPHERAL_BUSY;
    }

    /* 第三步：配置按键唤醒，再保存并暂停所有由开关选中的模块。 */
    gpio_status = HW_GPIO_INPUT_IT_init(LOWPOWER_WAKEUP_KEY_PIN,
                                        LOWPOWER_WAKEUP_KEY_PULL,
                                        LOWPOWER_WAKEUP_KEY_TRIGGER);
    if (gpio_status != HW_GPIO_STATUS_OK)
    {
        lowpower_restore_adc();
        HW_LowPower_RestoreCallback();
        __enable_irq();
        return HW_LOWPOWER_STATUS_WAKEUP_CONFIG_FAILED;
    }
    lowpower_suspend_peripherals();
    lowpower_suspend_timers();

    /* 第四步：选择Stop档位并等待唤醒，高速时钟由芯片自动停止。 */
    was_pwr_clock_enabled = LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_PWR);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    lowpower_configure_stop_mode();
    LL_LPM_DisableSleepOnExit();
    LL_LPM_EnableDeepSleep();
    __DSB();
    __WFI();
    __ISB();

    /* 第五步：唤醒后先退出DeepSleep并恢复系统主频。 */
    LL_LPM_EnableSleep();
    LL_PWR_DisableLowPowerRunMode();
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    APP_SystemClockConfig(LOWPOWER_RESTORE_CLOCK_MHZ);

    /* 第六步：先恢复普通外设和定时器，最后再开放全局中断。 */
    lowpower_resume_peripherals();
    lowpower_resume_timers();
    HW_LowPower_RestoreCallback();
    if (was_pwr_clock_enabled == 0U)
    {
        LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_PWR);
    }
    __enable_irq();
    __ISB();

    return HW_LOWPOWER_STATUS_OK;
}
