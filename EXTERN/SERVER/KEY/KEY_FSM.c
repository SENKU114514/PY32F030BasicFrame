#include "./SERVER/KEY/KEY_FSM.h"
#include "./HW_INIT/GPIO/GPIO_init.h"

#define APP_FSM_KEY_DELAY_TIME 	50U		//消抖时间
#define APP_FSM_KEY_DOWN_DELAY 100U		//多击间隔



/*上电获取按键默认态*/
KEY_FSM_State_e SERVER_KEY_FSM_Get_Default_State(KEY_FSM_Key_Info_t *KEY_Info){
	HW_GPIO_Status_e status;
	status = HW_GPIO_INPUT_init(KEY_Info->GPIO_index,NO);
	if(status != HW_GPIO_STATUS_OK){
		return KEY_FSM_GPIO_INIT_FAIL;
	}
	
	status = HW_GPIO_Get_SingleKey(KEY_Info->GPIO_index,&KEY_Info->Default_Value);
	if(status != HW_GPIO_STATUS_OK){
		return KEY_FSM_GPIO_GET_FAIL;
	}
	
	return KEY_FSM_OK;
}

/*FSM初始化函数*/
KEY_FSM_State_e SERVER_KEY_FSM_init(KEY_FSM_Key_Info_t *KEY_Info){
	KEY_FSM_State_e status;
	status = SERVER_KEY_FSM_Get_Default_State(KEY_Info);
	if(status != KEY_FSM_OK){
		return KEY_FSM_GPIO_INIT_FAIL;
	}
	
	return KEY_FSM_OK;
}


/*按键状态机函数，放入1ms*/
KEY_FSM_State_e SERVER_KEY_FSM_ALL_STATE(KEY_FSM_Key_Info_t *KEY_Info){
	HW_GPIO_Status_e status;
	uint8_t key_value;
	
	/*时间变化开始计时*/
	static uint8_t Times_Delay;
	if(KEY_Info->Key_Down_Times > 0){
		Times_Delay++;
	}
	
	
	/*超过规定时间，清除次数*/
	if(Times_Delay >= APP_FSM_KEY_DOWN_DELAY){
		KEY_Info->Key_Down_Times = 0;
		Times_Delay = APP_FSM_KEY_DOWN_DELAY+1;
	}
	
	if(KEY_Info->Key_Down_Times >= 2){
		KEY_Info->Key_Infomation = KEY_FSM_DOUBLE;
		return KEY_FSM_OK;
	}
	
	switch(KEY_Info->Key_Infomation){
		/*松开*/
		case KEY_FSM_UP:
			status = HW_GPIO_Get_SingleKey(KEY_Info->GPIO_index,&key_value);
			if(key_value != KEY_Info->Default_Value){
				KEY_Info->Key_Infomation = KEY_FSM_DOWN_DELAY;
				KEY_Info->Key_Delay_Time = 0;
			}
			break;
			
		/*按下消抖*/
		case KEY_FSM_DOWN_DELAY:
			KEY_Info->Key_Delay_Time++;
			status = HW_GPIO_Get_SingleKey(KEY_Info->GPIO_index,&key_value);
			Times_Delay = 0;
			if(key_value == KEY_Info->Default_Value){
				KEY_Info->Key_Infomation = KEY_FSM_UP;
				KEY_Info->Key_Delay_Time = 0;
			}
			else if(KEY_Info->Key_Delay_Time >= APP_FSM_KEY_DELAY_TIME){
				KEY_Info->Key_Infomation = KEY_FSM_DOWN;
				KEY_Info->Key_Delay_Time = 0;
			}
			break;
			
		/*按下*/
		case KEY_FSM_DOWN:
			status = HW_GPIO_Get_SingleKey(KEY_Info->GPIO_index,&key_value);
			if (key_value == KEY_Info->Default_Value)
			{
					KEY_Info->Key_Infomation = KEY_FSM_UP_DELAY;
					KEY_Info->Key_Delay_Time = 0;
			}
			break;

    /* 松开消抖 */
    case KEY_FSM_UP_DELAY:
				KEY_Info->Key_Delay_Time++;
				status = HW_GPIO_Get_SingleKey(KEY_Info->GPIO_index,&key_value);
				Times_Delay = 0;
        if (key_value != KEY_Info->Default_Value)
        {
            /* 松开期间又按下，继续认为按住 */
            KEY_Info->Key_Infomation = KEY_FSM_DOWN;
            KEY_Info->Key_Delay_Time = 0;
        }
        else if (KEY_Info->Key_Delay_Time >= APP_FSM_KEY_DELAY_TIME)
        {
            /* 连续松开达到消抖时间：确认松开 */
            KEY_Info->Key_Infomation = KEY_FSM_UP;
            KEY_Info->Key_Delay_Time = 0;
						KEY_Info->Key_Down_Times++;		//松开代表完成一次按键
        }
        break;
				
		/* 容错处理 */
		default:
					KEY_Info->Key_Infomation = KEY_FSM_UP;
					KEY_Info->Key_Delay_Time = 0;
					break;
	}
	
	return KEY_FSM_OK;
}

