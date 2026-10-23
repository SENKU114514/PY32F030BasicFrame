#ifndef __BSP_UART_H__
#define __BSP_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 软件串口状态 */
typedef enum
{
	BSP_UART_OK = 0,			//成功
	BSP_UART_FAIL,				//失败
}BSP_UART_State_e;

/* 初始化：指定发送引脚、接收引脚和波特率 */
BSP_UART_State_e BSP_UART_INIT(GPIO_index_e tx_pin, GPIO_index_e rx_pin, uint32_t baud);

/* 发送：使用定时器精确定时 */
BSP_UART_State_e BSP_UART_TIM_SEND(const uint8_t *data, uint16_t length);

/* 发送：使用软件循环定时，不占用定时器 */
BSP_UART_State_e BSP_UART_SOFT_SEND(const uint8_t *data, uint16_t length);

/* 接收：使用定时器采样，阻塞到收满或超时 */
uint16_t BSP_UART_RECEIVE(uint8_t *data, uint16_t capacity, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
