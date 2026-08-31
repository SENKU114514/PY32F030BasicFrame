#include "./HW_INIT/IWDG/IWDG_Init.h"

/**
  * @brief  HW_IWDG_Init
  * @param  None
  * @retval None
  */
void HW_IWDG_Init(uint8_t FeedTime)
{
  /* 使能LSI */
  LL_RCC_LSI_Enable();
  while (LL_RCC_LSI_IsReady() == 0U) {;}

  /* 使能IWDG */
  LL_IWDG_Enable(IWDG);
  
  /* 开启写权限 */
  LL_IWDG_EnableWriteAccess(IWDG);
 
  /* 设置IWDG分频 */
  LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_32); /*LSI=32.768K   T=1MS */
  
  /* 设置喂狗事件*/
  LL_IWDG_SetReloadCounter(IWDG, FeedTime); /* 1ms*1000=1s */
 
  /* IWDG初始化*/
  while (LL_IWDG_IsReady(IWDG) == 0U) {;}
 
  /* 喂狗 */
  LL_IWDG_ReloadCounter(IWDG);
		
	LOG_DEBUG("IWDG init:%d\r\n",FeedTime);
}

