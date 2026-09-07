/**/

#include "app_main.h"

#define LED_PIN A3
#define KEY_PIN A5
#define LOWPOWER_IDLE_TIME_MS 20000U

static uint8_t lowpower_key_default_level;
static uint32_t lowpower_last_activity_tick;

/* 执行一次按键活动检测和按键 FSM 处理。 */
static void app_key_fsm_schedule_task(void *p_context)
{
	uint8_t key_level;

	(void)p_context;

	if((HW_GPIO_Get_SingleKey(KEY_PIN, &key_level) == HW_GPIO_STATUS_OK) &&
		 (key_level != lowpower_key_default_level)){
		lowpower_last_activity_tick = get_mag_tick();
	}

	APP_INPUT_KEY1();
}

#ifdef __IWDG_INIT_H__
/* 执行一次独立看门狗喂狗操作。 */
static void app_watchdog_schedule_task(void *p_context)
{
	(void)p_context;
	FEED_DOG;
}
#endif

/* 新增任务只需在表里加一行；函数定义放在此表之前。时间单位：ms */
static const MAG_TaskTableEntry_t s_app_tasks[] =
{
	/* 执行函数          执行间隔 首次延迟   执行次数 */
	{app_key_fsm_schedule_task,1U,0U,MAG_SCHEDULER_RUN_FOREVER},
	
	
#ifdef __IWDG_INIT_H__
	{app_watchdog_schedule_task,50U,50U,MAG_SCHEDULER_RUN_FOREVER},
#endif
};

/* 初始化应用模块及新版 MAG 调度器。 */
void app_main_init(void){
	MAG_SchedulerStatus_e scheduler_status;

	/*LOG*/
	#if LOG_ENABLE
	Log_init();
	#endif
	
	/*业务输入初始化*/
	APP_Input_init();
	HW_GPIO_INPUT_IT_init(KEY_PIN, NO, GPIO_IT_RISING_FALLING);
	HW_GPIO_Get_SingleKey(KEY_PIN, &lowpower_key_default_level);
	
	/*业务输出初始化*/
	APP_Output_init();
	
	/*业务通信初始化*/
	APP_Comm_init();

	/* 使用下方休眠示例前，先初始化空闲唤醒按键，并检查返回状态。 */
//	HW_LowPower_init(B2, UP, LOW); // 示例：B2 按下接地，连续长按 3 秒唤醒

	/* 业务模块准备完成后，一次初始化自动装入整张任务表。 */
	scheduler_status = MAG_SchedulerInit(s_app_tasks,
		sizeof(s_app_tasks) / sizeof(s_app_tasks[0]));
	if(scheduler_status != MAG_SCHEDULER_STATUS_OK){
		LOG_DEBUG("scheduler init failed: %d\r\n", (int)scheduler_status);
	}
	lowpower_last_activity_tick = get_mag_tick();

	/* 喂狗任务准备成功后才启动不可停止的独立看门狗。 */
	#ifdef __IWDG_INIT_H__
	if(scheduler_status == MAG_SCHEDULER_STATUS_OK){
		HW_IWDG_Init(70);//100ms喂一次
	}
	#endif
}

/* 驱动新版 MAG 调度器并处理低功耗业务。 */
void app_main(void){
	/*任务调度*/
	MAG_SchedulerProcess();
	
	//休眠示例
//	HW_LowPower_Status_e lowpower_status;
//
//	if((uint32_t)(get_mag_tick() - lowpower_last_activity_tick) >= LOWPOWER_IDLE_TIME_MS){
//		lowpower_last_activity_tick = get_mag_tick();
//		APP_Output_LED1(1U);
//		APP_Output_LED2(1U);
//		lowpower_status = HW_LowPower_Enter();
//		if(lowpower_status == HW_LOWPOWER_STATUS_OK){
//			lowpower_last_activity_tick = get_mag_tick();
//			APP_Output_LED1(0U);
//		}
//	}
}
