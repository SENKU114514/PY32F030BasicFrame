/*非阻塞任务调度*/

//问题：（低容错）某个代码运行超过1ms的时候，代码会有风险，没办法每次都跑进1ms任务，甚至可能无法喂狗

#include "mag_task.h"

uint32_t mag_tick;
uint32_t mag_old_tick;

/*任务调度初始化*/
uint8_t mag_task_init(void)
{
	if(mag_tick_init() != MAG_TICK_INIT_OK){//初始化(定时器)
		LOG_DEBUG("legacy MAG tick init failed\r\n");
		return MAG_TASK_INIT_FAILED;
	}
	mag_tick = get_mag_tick();//获取时间
	mag_old_tick = mag_tick;//更新时间
	LOG_DEBUG("task init success\r\n");
	return MAG_TASK_INIT_OK;
}

/*任务跳转位置*/
__weak void task_1ms()	{}
__weak void task_50ms()	{}
__weak void task_100ms(){}
__weak void task_500ms(){}
__weak void task_1s()		{}

/*任务管理*/
void mag_task(){
	mag_tick = get_mag_tick();//更新状态
	
	uint32_t mag_diff;//差值
	/*判断是否更新*/
	if(mag_tick != mag_old_tick)
	{
		mag_diff = mag_tick - mag_old_tick;//无符号减法会自动处理 32 位 tick 回绕
		mag_old_tick = mag_tick;//时间变化，更新旧时间
	
		/*判断任务执行时机*/
		static uint16_t task_50ms_time = 0;
		static uint16_t task_100ms_time = 0;
		static uint16_t task_500ms_time = 0;
		static uint16_t task_1s_time = 0;
		
		//1ms任务
		task_1ms();
		
		//50ms任务
		task_50ms_time += mag_diff;
		if(task_50ms_time >= 50){
			task_50ms();
			task_50ms_time = 0;
		}
		
		//100ms任务
		task_100ms_time += mag_diff;
		if(task_100ms_time >= 100){
			task_100ms();
			task_100ms_time = 0;
		}
		
		//500ms任务
		task_500ms_time += mag_diff;
		if(task_500ms_time >= 500){
			task_500ms();
			task_500ms_time = 0;
		}
		
		//1s任务
		task_1s_time += mag_diff;
		if(task_1s_time >= 1000){
			task_1s();
			task_1s_time = 0;
		}
	}
}

