#include "system_init.h"

/**
  * @brief  系统时钟配置函数
  * @param  无
  * @retval 无
  */
void APP_SystemClockConfig(uint8_t FRE)
{
  /* 使能HSI */
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1){}
	
	switch(FRE)
	{
		case 4: 	LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_4MHz);break;//将内部频率设定为4M
		case 8: 	LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_8MHz);break;//将内部频率设定为8M
		case 16: 	LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_16MHz);break;//将内部频率设定为24M
		case 24: 	LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_24MHz);break;//将内部频率设定为24M
		case 48:	LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_24MHz);break;//将内部频率设定为24M,后续PLL倍频
		default: 	LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_24MHz);break;
	}
	
  /* 设置 AHB 分频*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

	if(FRE == 48){
		/*配置PLL*/
		LL_RCC_PLL_Disable();                            //关闭PLL
		LL_RCC_PLL_SetMainSource(LL_RCC_PLLSOURCE_HSI);  //设置PLL来源
		LL_RCC_PLL_Enable();                             //开启PLL
		while(LL_RCC_PLL_IsReady() != 1){};							 //等待PLL稳定
		
		//48n内核必须配置1个Flash等待周期
		LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
		while (LL_FLASH_GetLatency () != LL_FLASH_LATENCY_1) ;
		
		/* 配置PLL作为系统时钟源 */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
		while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL){}
	}
	else{
		/* 配置HSISYS作为系统时钟源 */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSISYS);
		while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS){}
	}

  /* 设置 APB1 分频*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_Init1msTick(FRE*1000000);

  /* 更新系统时钟全局变量SystemCoreClock(也可以通过调用SystemCoreClockUpdate函数更新) */
  LL_SetSystemCoreClock(FRE*1000000);
}

