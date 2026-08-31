#ifndef EXTERN_MAG_TASK_H
#define EXTERN_MAG_TASK_H

#include "main.h"
#include "mag_tick.h"							//时钟来源


/*配置引脚结构体*/


/*命名规则:HW_GPIO_类型_名字，类型全大写，名称开头大写，后小写*/

/*任务跳转位置*/
__weak void task_1ms	(void);
__weak void task_50ms	(void);
__weak void task_100ms(void);
__weak void task_500ms(void);
__weak void task_1s		(void);

#define MAG_TASK_INIT_OK       0U
#define MAG_TASK_INIT_FAILED   1U

uint8_t mag_task_init(void);
void mag_task(void);//任务管理


#endif /* EXTERN_MAG_TASK_H */


