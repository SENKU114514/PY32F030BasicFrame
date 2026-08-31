#ifndef __GPIO_INIT_H__
#define __GPIO_INIT_H__

#include "./HW_INIT/GPIO/GPIO_types.h"

/*命名规则:HW_GPIO_类型_名字，类型全大写，名称开头大写，后小写*/

typedef enum
{
    LOW,
    HIGH,
    TOGGLE,
} GPIO_mode_e;

typedef enum
{
    NO = 0,
    UP,
    DOWN,
} GPIO_pull_e;

/* GPIO external interrupt edge trigger mode. */
typedef enum
{
    GPIO_IT_RISING,
    GPIO_IT_FALLING,
    GPIO_IT_RISING_FALLING,
} GPIO_IT_Trigger_e;

/* GPIO hardware layer status. */
typedef enum
{
    HW_GPIO_STATUS_OK = 0,
    HW_GPIO_STATUS_INVALID_ARG,
    HW_GPIO_STATUS_UNSUPPORTED,
    HW_GPIO_STATUS_INIT_FAILED,
    HW_GPIO_STATUS_IT_CONFLICT,
} HW_GPIO_Status_e;

/*输出*/
HW_GPIO_Status_e HW_GPIO_OUT_init(GPIO_index_e GPIO_index,GPIO_pull_e pull);//初始化输出GPIO
HW_GPIO_Status_e HW_GPIO_SET_Pin(GPIO_index_e GPIO_index,GPIO_mode_e mode);//设置电平模式

/*输入*/
HW_GPIO_Status_e HW_GPIO_INPUT_init(GPIO_index_e GPIO_index,GPIO_pull_e pull);//初始化输入GPIO
HW_GPIO_Status_e HW_GPIO_Get_SingleKey(GPIO_index_e GPIO_index,uint8_t *out_level);//获取单按键值

/*
 * 初始化 GPIO 外部中断输入。
 * 当前固定使用无上下拉输入；后续需要时可在不改变映射表的前提下扩展 pull 参数。
 */
HW_GPIO_Status_e HW_GPIO_INPUT_IT_init(GPIO_index_e GPIO_index,
																			 GPIO_pull_e pull,
                                       GPIO_IT_Trigger_e trigger);

/*
 * GPIO 外部中断的默认回调出口。
 * APP 层可用同名非 weak 函数覆盖它，并通过 GPIO_index 判断触发引脚。
 */
void HW_GPIO_INPUT_IT_Callback(GPIO_index_e GPIO_index);


#endif


