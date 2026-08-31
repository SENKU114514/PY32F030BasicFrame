#ifndef __IWDG_INIT_H__
#define __IWDG_INIT_H__


#include "main.h"
 
#define FEED_DOG LL_IWDG_ReloadCounter(IWDG)
 
void HW_IWDG_Init(uint8_t FeedTime);//看门狗初始化
 

#endif


