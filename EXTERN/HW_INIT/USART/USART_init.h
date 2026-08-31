#ifndef __USART_INIT_H__
#define __USART_INIT_H__

#include "main.h"

/* UART 硬件层：配置合法的 TX/RX 引脚组合，并提供简单的字符串发送接口。 */

//外设配置相关定义
typedef enum{
    UART1,
		UART2,
    UART_COUNT,
}UART_index_e;

/*TX引脚*/
typedef enum
{
    TX_A0,     /* USART2_TX，AF9  */
    TX_A2,     /* USART1_TX，AF1；USART2_TX，AF4 */
    TX_A4,     /* USART2_TX，AF9  */
    TX_A7,     /* USART1_TX，AF8；USART2_TX，AF9 */
    TX_A9,     /* USART1_TX，AF1；USART2_TX，AF4 */
    TX_A10,    /* USART1_TX，AF8 */
    TX_A14,    /* USART1_TX，AF1；USART2_TX，AF4 */

    TX_B6,     /* USART1_TX，AF0；USART2_TX，AF4 */
    TX_B8,     /* USART1_TX，AF8；USART2_TX，AF4 */

    TX_F0,     /* USART2_TX，AF9 */
    TX_F1,     /* USART1_TX，AF8；USART2_TX，AF4 */
    TX_F3,     /* USART1_TX，AF0；USART2_TX，AF4 */
} UART_TX_Pin_e;

/*RX引脚*/
typedef enum
{
    RX_A1,     /* USART2_RX，AF9 */
    RX_A3,     /* USART1_RX，AF1；USART2_RX，AF4 */
    RX_A5,     /* USART2_RX，AF9 */
    RX_A8,     /* USART1_RX，AF8；USART2_RX，AF9 */
    RX_A9,     /* USART1_RX，AF8 */
    RX_A10,    /* USART1_RX，AF1；USART2_RX，AF4 */
    RX_A13,    /* USART1_RX，AF8 */
    RX_A15,    /* USART1_RX，AF1；USART2_RX，AF4 */

    RX_B2,     /* USART1_RX，AF0；USART2_RX，AF3 */
    RX_B7,     /* USART1_RX，AF0；USART2_RX，AF4 */

    RX_F0,     /* USART1_RX，AF8；USART2_RX，AF4 */
    RX_F1,     /* USART2_RX，AF9 */
    RX_F2,     /* USART2_RX，AF4 */
} UART_RX_Pin_e;

/* UART 操作状态：供 Service 层判断初始化或发送是否成功。 */
typedef enum
{
    HW_UART_STATUS_OK = 0,
    HW_UART_STATUS_INVALID_ARG,
    HW_UART_STATUS_UNSUPPORTED,
    HW_UART_STATUS_GPIO_INIT_FAILED,
    HW_UART_STATUS_INIT_FAILED,
    HW_UART_STATUS_NOT_READY,
    HW_UART_STATUS_TIMEOUT,
    HW_UART_STATUS_NO_DATA,
    HW_UART_STATUS_BUFFER_OVERFLOW,
}HW_UART_Status_e;

/* 初始化 UART：参数顺序为 UART 实例、TX 引脚、RX 引脚、波特率。 */
HW_UART_Status_e HW_UART_init(UART_index_e UART_index,
                               UART_TX_Pin_e TX_Pin,
                               UART_RX_Pin_e RX_Pin,
                               uint32_t BaudRate);

/* 按指定长度发送数据，可以发送字符串或包含 0x00 的二进制数据。 */
HW_UART_Status_e HW_UART_Send(UART_index_e UART_index,
                              const uint8_t *data,
                              uint16_t data_length);

/*
 * 使用中断缓冲区接收数据。
 * data_length 调用前表示 data 的容量，返回后表示本次实际取出的字节数。
 */
HW_UART_Status_e HW_UART_Receive_IT(UART_index_e UART_index,
                                    uint8_t *data,
                                    uint16_t *data_length);


#endif
