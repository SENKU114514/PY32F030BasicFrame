/**/

#include "app_main.h"

#define LED_PIN A3
#define KEY_PIN A5
#define LOWPOWER_IDLE_TIME_MS 20000U

static uint8_t lowpower_key_default_level;
static uint32_t lowpower_last_activity_tick;

#if defined(MAG_USE_SCHEDULER)
static MAG_TaskHandle_t s_key_fsm_task_handle;
#ifdef __IWDG_INIT_H__
static MAG_TaskHandle_t s_watchdog_task_handle;
static uint8_t s_watchdog_task_is_ready;
#endif
#endif

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

#if defined(MAG_USE_SCHEDULER)
/* 把示例任务注册到新版无序链表调度器。 */
static void app_register_scheduler_tasks(void)
{
	MAG_TaskConfig_t task_config;
	MAG_SchedulerStatus_e status;

	/* 一：把按键 FSM 注册为 1 ms 永久任务。 */
	task_config.callback = app_key_fsm_schedule_task;
	task_config.p_context = NULL;
	task_config.first_delay_ms = 0U;
	task_config.period_ms = 1U;
	task_config.run_count = MAG_SCHEDULER_RUN_FOREVER;

	status = MAG_SchedulerRegister(&task_config, &s_key_fsm_task_handle);
	if(status != MAG_SCHEDULER_STATUS_OK){
		LOG_DEBUG("register key FSM task failed: %d\r\n", (int)status);
	}

	#ifdef __IWDG_INIT_H__
	/* 二：看门狗启用时，把喂狗注册为 50 ms 永久任务。 */
	s_watchdog_task_is_ready = 0U;
	task_config.callback = app_watchdog_schedule_task;
	task_config.p_context = NULL;
	task_config.first_delay_ms = 50U;
	task_config.period_ms = 50U;
	task_config.run_count = MAG_SCHEDULER_RUN_FOREVER;

	status = MAG_SchedulerRegister(&task_config, &s_watchdog_task_handle);
	if(status != MAG_SCHEDULER_STATUS_OK){
		LOG_DEBUG("register watchdog task failed: %d\r\n", (int)status);
	}
	else{
		s_watchdog_task_is_ready = 1U;
	}
	#endif
}
#endif

/* 初始化应用模块并选择旧版或新版 MAG 调度模式。 */
void app_main_init(void){
	#if defined(MAG_USE_SCHEDULER)
	MAG_SchedulerStatus_e scheduler_status;
	#endif
	#if defined(MAG_USE_LEGACY_TASK) && defined(__IWDG_INIT_H__)
	uint8_t legacy_mag_status;
	#endif

	/*LOG*/
	#if LOG_ENABLE
	Log_init();
	#endif
	
	/*任务调度*/
	#if defined(MAG_USE_LEGACY_TASK)
	#ifdef __IWDG_INIT_H__
	legacy_mag_status = mag_task_init();
	#else
	(void)mag_task_init();
	#endif
	#else
	scheduler_status = MAG_SchedulerInit();
	if(scheduler_status != MAG_SCHEDULER_STATUS_OK){
		LOG_DEBUG("scheduler init failed: %d\r\n", (int)scheduler_status);
	}
	#endif
	
	/*业务输入初始化*/
	APP_Input_init();
	HW_GPIO_INPUT_IT_init(KEY_PIN, NO, GPIO_IT_RISING_FALLING);
	HW_GPIO_Get_SingleKey(KEY_PIN, &lowpower_key_default_level);
	lowpower_last_activity_tick = get_mag_tick();
	
	/*业务输出初始化*/
	APP_Output_init();
	
	/*业务通信初始化*/
	APP_Comm_init();

	/*新版模式在所有业务模块初始化完成后注册计划任务。*/
	#if defined(MAG_USE_SCHEDULER)
	app_register_scheduler_tasks();
	#endif

	/* 喂狗任务准备成功后才启动不可停止的独立看门狗。 */
	#ifdef __IWDG_INIT_H__
	#if defined(MAG_USE_LEGACY_TASK)
	if(legacy_mag_status == MAG_TASK_INIT_OK){
		HW_IWDG_Init(70);//100ms喂一次
	}
	#else
	if(s_watchdog_task_is_ready != 0U){
		HW_IWDG_Init(70);//100ms喂一次
	}
	#endif
	#endif
	
	
	/* 初始化输出GPIO */
//	HW_GPIO_OUT_init(LED_PIN,NO);//初始化A8输入引脚

//  /* 初始化输入GPIO */
//	HW_GPIO_INPUT_init(KEY_PIN,NO);//初始化A0引脚
//	
//	/* 外部触发中断GPIO*/
//	HW_GPIO_INPUT_IT_init(A5,UP,GPIO_IT_RISING);

	/* 初始化UART */
//	HW_UART_init(UART2, TX_A4, RX_A5, 115200U);

	/* 初始化SPI */
//	HW_SPI_DMA_init(SPI_BUS1, SCK_A1, MOSI_A2, MISO_NULL);//配置参数为SCK.PA1,MOSI.PA2，MISO未使用

	/* 初始化I2C */
//	HW_I2C_init(I2C_BUS1,SCL_A3,SDA_B7,100000U);
}

/* 驱动当前 MAG 调度模式并处理低功耗业务。 */
void app_main(void){
	HW_LowPower_Status_e lowpower_status;

	/*任务调度*/
	#if defined(MAG_USE_LEGACY_TASK)
	mag_task();
	#else
	MAG_SchedulerProcess();
	#endif

	//休眠示例
	if((uint32_t)(get_mag_tick() - lowpower_last_activity_tick) >= LOWPOWER_IDLE_TIME_MS){
		lowpower_last_activity_tick = get_mag_tick();
		APP_Output_LED1(1U);
		APP_Output_LED2(1U);
		lowpower_status = HW_LowPower_Enter();
		if(lowpower_status == HW_LOWPOWER_STATUS_OK){
			lowpower_last_activity_tick = get_mag_tick();
			APP_Output_LED1(0U);
		}
	}
}

#if defined(MAG_USE_LEGACY_TASK)
/* 旧版 MAG 的 1 ms 任务入口。 */
void task_1ms(void)
{
	app_key_fsm_schedule_task(NULL);
}

/* 旧版 MAG 的 50 ms 任务入口。 */
void task_50ms(void){
	#ifdef __IWDG_INIT_H__
	app_watchdog_schedule_task(NULL);
	#endif
}

/* 旧版 MAG 预留的 100 ms 任务入口。 */
void task_100ms(void){

}

/* 旧版 MAG 预留的 500 ms 任务入口。 */
void task_500ms(void){

}

/* 旧版 MAG 预留的 1 s 任务入口。 */
void task_1s(void)
{
	
}
#endif
