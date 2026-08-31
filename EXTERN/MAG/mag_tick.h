#ifndef EXTERN_MAG_TICK_H
#define EXTERN_MAG_TICK_H

#include "main.h"



/*配置引脚结构体*/


/*命名规则:HW_GPIO_类型_名字，类型全大写，名称开头大写，后小写*/


#define MAG_TICK_INIT_OK       0U
#define MAG_TICK_INIT_FAILED   1U

uint8_t mag_tick_init(void);/* 初始化定时器 */

uint32_t get_mag_tick(void);//获取时钟参数

#endif /* EXTERN_MAG_TICK_H */


