#ifndef __IR_RECEIVE_H__
#define __IR_RECEIVE_H__

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif


/* 接收状态，只有 OK 表示本次取到了校验通过的数据。 */
typedef enum
{
  SERVER_IR_OK = 0,
	SERVER_IR_IT_INIT_FAIL,
  SERVER_IR_FAIL,
} SERVER_IR_Status_e;

//初始化
SERVER_IR_Status_e Server_IR_init(TIM_TypeDef *TIMx_init,GPIO_index_e GPIO_index_init);

void Server_IR_ChangeReceiveLen(uint8_t num);		//改变接受长度
void Server_IR_Data_Handle(void *context);			//原始信息处理为字节

void Server_IR_Data_Return(uint8_t *arry, uint8_t *num);//返回数据参数与长度

#ifdef __cplusplus
}
#endif

#endif
