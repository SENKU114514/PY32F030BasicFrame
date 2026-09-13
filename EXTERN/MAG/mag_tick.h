#ifndef EXTERN_MAG_TICK_H
#define EXTERN_MAG_TICK_H

#include "main.h"

/* TIM_init.c supplies the TIM1 vector by default. Set to 1 project-wide
 * only when an existing vector calls MAG_TickIRQHandler() explicitly. */
#ifndef MAG_TICK_EXTERNAL_IRQ
#define MAG_TICK_EXTERNAL_IRQ 0
#endif

#if defined(PY32F002BPRE)
void MAG_TickIRQHandler(void);
#endif


/*配置引脚结构体*/


/*命名规则:HW_GPIO_类型_名字，类型全大写，名称开头大写，后小写*/


#define MAG_TICK_INIT_OK       0U
#define MAG_TICK_INIT_FAILED   1U

uint8_t mag_tick_init(void);/* 初始化定时器 */

uint32_t get_mag_tick(void);//获取时钟参数

#endif /* EXTERN_MAG_TICK_H */


