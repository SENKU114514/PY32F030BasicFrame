/**
  ******************************************************************************
  * @file    main.h
  * @author  MCU Application Team
  * @brief   Header for main.c file.
  *          This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Dependent------------------------------------------------------------------*/
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

/* Includes ------------------------------------------------------------------*/
#include "py32f0xx_ll_rcc.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_system.h"
#include "py32f0xx_ll_cortex.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_pwr.h"
#include "py32f0xx_ll_dma.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_ll_spi.h"
#include "py32f0xx_ll_tim.h"
#include "py32f0xx_ll_iwdg.h"
#include "py32f0xx_ll_usart.h"
#include "py32f0xx_ll_i2c.h"
#include "py32f0xx_ll_exti.h"

/*-----------------------------------MY_CODE-------------------------------------------------*/
/*系统相关初始化*/
#include "./SYS/system_init.h"				//系统时钟
//#include "./HW_INIT/IWDG/IWDG_Init.h"   //看门狗

/*HW_init------------------------------------------------------------------------------------*/
#include "./HW_INIT/GPIO/GPIO_init.h"   //GPIO
#include "./HW_INIT/SPI/SPI_init.h"     //SPI
#include "./HW_INIT/TIM/TIM_init.h"     //定时器
#include "./HW_INIT/ADC/ADC_init.h"			//ADC
#include "./HW_INIT/USART/USART_init.h"	//UART
#include "./HW_INIT/I2C/I2C_init.h"			//I2C
#include "./HW_INIT/LOWPOWER/LowPower.h" //低功耗

/*SERVER------------------------------------------------------------------------------------*/
#include "./SERVER/LOG/LOG.h"

/*任务管理-----------------------------------------------------------------------------------*/
//#include "RTOS.h"               			//实时操作系统

/* MAG 两种模式只能启用一行：注释当前行，再取消另一行的注释即可切换。 */
#define MAG_USE_LEGACY_TASK                      //旧版固定 1/50/100/500/1000 ms 调度
//#define MAG_USE_SCHEDULER                          //新版无序链表计划调度器

#if defined(MAG_USE_LEGACY_TASK) && defined(MAG_USE_SCHEDULER)
#error "Only one MAG scheduler mode can be enabled"
#elif !defined(MAG_USE_LEGACY_TASK) && !defined(MAG_USE_SCHEDULER)
#error "One MAG scheduler mode must be enabled"
#endif

#include "./MAG/mag_tick.h"                       //两种模式共用的 1 ms 时基

#if defined(MAG_USE_LEGACY_TASK)
#include "./MAG/mag_task.h"                       //旧版非阻塞分时任务
#else
#include "./MAG/mag_scheduler.h"                  //新版无序链表计划调度器
#endif

/*APP-----------------------------------------------------------------------------------------*/
#include "./APP/app_main.h"						//业务函数
#include "./APP/APP_INPUT/APP_input.h"			//业务输入
#include "./APP/APP_OUTPUT/APP_output.h"		//业务输出
#include "./APP/APP_COMM/APP_comm.h"				//业务通信

/*SERVER------------------------------------------------------------------------------------------*/
#include "./SERVER/LOG/LOG.h"							//日志
#include "./SERVER/KEY/KEY_FSM.h"					//按键状态机




#if defined(USE_FULL_ASSERT)
#include "py32_assert.h"
#endif /* USE_FULL_ASSERT */

/* Private includes ----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Exported variables prototypes ---------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
