/*时钟基准，使用中断定时*/

#include "mag_tick.h"

static volatile uint32_t s_mag_tick = 0xFFFFF000U;

/*初始化定时器*/
uint8_t mag_tick_init(void){
	/* 初始化定时器并把底层结果交给调用者。 */
	return (HW_TIM_COUNT_IT_init(TIM1, DOWN, 1U) == HW_TIM_STATUS_OK) ?
		MAG_TICK_INIT_OK : MAG_TICK_INIT_FAILED;//定时1ms
}

/*中断启动后自动对系统时间+1*/
void TIM1_UpdateCallback(void)//TIM1中断返回
{
	s_mag_tick++;
}

/* 获取当前 1 ms 系统时基。 */
uint32_t get_mag_tick()
{
	return s_mag_tick;
}

