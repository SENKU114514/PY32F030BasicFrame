#ifndef __KEY_FSM_H__
#define __KEY_FSM_H__

#ifdef __cplusplus
extern "C" {
#endif


/*code*/
#include "./HW_INIT/GPIO/GPIO_types.h"

/*按键状态*/
typedef enum{
	KEY_FSM_UP = 0,
	KEY_FSM_DOWN_DELAY,
	KEY_FSM_DOWN,
	KEY_FSM_UP_DELAY,
	KEY_FSM_DOUBLE,
	KEY_FSM_PUSH_LONG,
}KEY_FSM_KeyState_e;

typedef enum{
	KEY_FSM_OK,
	KEY_FSM_GPIO_INIT_FAIL,
	KEY_FSM_GPIO_GET_FAIL,
	KEY_FSM_FAIL,
}KEY_FSM_State_e;

/*按键信息*/
typedef struct{
	GPIO_index_e GPIO_index;               //按键引脚
	KEY_FSM_KeyState_e Key_Infomation;     //按键状态
	uint8_t Default_Value;                 //默认电平
	uint32_t Key_Delay_Time;               //延时时间
	uint8_t Key_Down_Times;                //按下次数
}KEY_FSM_Key_Info_t;

/**/
KEY_FSM_State_e SERVER_KEY_FSM_init(KEY_FSM_Key_Info_t *KEY_Info);				//初始化
KEY_FSM_State_e SERVER_KEY_FSM_ALL_STATE(KEY_FSM_Key_Info_t *KEY_Info);		//按键状态机



#ifdef __cplusplus
}
#endif

#endif


