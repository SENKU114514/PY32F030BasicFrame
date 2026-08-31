/**
  ******************************************************************************
  * @file    main.c
  * @author  MCU Application Team
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) Puya Semiconductor Co.
  * All rights reserved.</center></h2>
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "py32f030xx_ll_Start_Kit.h"



/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  应用程序入口函数.
  * @param  无
  * @retval int
  */
int main(void)
{
	/* 配置系统时钟，延时500ms，防止设置错误导致无法烧写*/
  APP_SystemClockConfig(24);//频率为24M
	LL_mDelay(500);
	
  /* 配置系统时钟 */
  APP_SystemClockConfig(48);//频率为48M
	
	/* 初始化应用；新版 MAG 会在这里建立链表并注册按键 FSM。 */
	app_main_init();
	
  while (1)
  {
		/* 每轮驱动选中的 MAG 模式，并执行已经到期的任务。 */
		app_main();
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  输出产生断言错误的源文件名及行号
  * @param  file：源文件名指针
  * @param  line：发生断言错误的行号
  * @retval 无
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* 用户可以根据需要添加自己的打印信息,
     例如: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* 无限循环 */
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
