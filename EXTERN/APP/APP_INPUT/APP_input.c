#include "./APP/APP_INPUT/APP_input.h"

#define KEY_PIN A5

static KEY_FSM_Key_Info_t KEY1;

/* 初始化输入业务所需的 GPIO。 */
APP_INPUT_State_e APP_Input_init(void)
{
//    HW_GPIO_Status_e status;

//    status = HW_GPIO_INPUT_init(ZERO_AC, NO);
//    if (status != HW_GPIO_STATUS_OK)
//    {
//        return APP_INPUT_ZERO_AC_FAIL;
//    }
		
		KEY1.GPIO_index = KEY_PIN;//引脚初始化
		SERVER_KEY_FSM_init(&KEY1);

    return APP_INPUT_OK;
}

void HW_GPIO_INPUT_IT_Callback(GPIO_index_e GPIO_index)
{
    switch (GPIO_index)
    {
        case A5:
            /* A5 的中断处理 */
            break;

        case B2:
            /* B2 的中断处理 */
            break;

        default:
            break;
    }
}

void APP_INPUT_KEY1(void){
	/*按键状态机单击双击测试*/
	KEY_FSM_State_e status;
	status = SERVER_KEY_FSM_ALL_STATE(&KEY1);
	
	switch((uint8_t)KEY1.Key_Infomation){
		case KEY_FSM_DOWN:
			APP_Output_LED1(1);
			APP_Output_LED2(0);
			break;
		case KEY_FSM_DOUBLE:
			APP_Output_LED1(0);
			break;
		case KEY_FSM_UP:
			APP_Output_LED2(1);
			break;
	}
	
}
	





