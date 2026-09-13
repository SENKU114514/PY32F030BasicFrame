/* Shared TIM1 millisecond time base for both device branches. */
#include "mag_tick.h"
#include "../HW_INIT/TIM/TIM_init.h"
#include "../HW_INIT/GPIO/GPIO_init.h"
static volatile uint32_t s_mag_tick = 0xFFFFF000U;
uint8_t mag_tick_init(void)
{
    return (HW_TIM_COUNT_IT_init(TIM1, UP, 1U) == HW_TIM_STATUS_OK) ?
        MAG_TICK_INIT_OK : MAG_TICK_INIT_FAILED;
}
void TIM1_UpdateCallback(void) { s_mag_tick++; }
#if defined(PY32F002BPRE)
void MAG_TickIRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM1) && LL_TIM_IsEnabledIT_UPDATE(TIM1))
    {
        LL_TIM_ClearFlag_UPDATE(TIM1);
        TIM1_UpdateCallback();
    }
}
#endif
uint32_t get_mag_tick(void) { return s_mag_tick; }
