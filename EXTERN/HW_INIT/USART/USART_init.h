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

/* UART 操作状态：供上层判断初始化、发送或接收是否成功。 */
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
    HW_UART_STATUS_BUSY,
    HW_UART_STATUS_RX_ERROR,
    HW_UART_STATUS_INVALID_CONTEXT,
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
 * 一次性中断接收（同步阻塞）。
 *
 * 每次调用都从 data[0] 开始保存本轮新收到的数据，最多保存
 * data_capacity 个字节。收到首字节后，如果连续 silence_timeout_ms
 * 没有新字节，则认为本轮不定长回复结束并直接返回。
 *
 * timeout_ms 是整个接收过程的最长等待时间；函数返回后，后续字节
 * 不会再写入本次 data。received_length 返回本次实际保存的字节数。
 * 一问一答时，应在 HW_UART_Send 返回后立即调用本函数。
 * 一次性窗口内的字节不会同时写入环形缓冲区。
 *
 * 本函数只能在线程/主循环中、全局中断开启时调用。即使返回
 * TIMEOUT、BUFFER_OVERFLOW 或 RX_ERROR，received_length 仍会返回
 * 已经保存的部分数据；错误优先级为 RX_ERROR、BUFFER_OVERFLOW、TIMEOUT。
 */
HW_UART_Status_e HW_UART_Receive_IT(UART_index_e UART_index,
                                    uint8_t *data,
                                    uint16_t data_capacity,
                                    uint16_t *received_length,
                                    uint32_t timeout_ms,
                                    uint32_t silence_timeout_ms);

/*
 * 读取持续接收的中断环形缓冲区。
 * data_length 调用前表示 data 的容量，返回后表示本次实际取出的字节数。
 * 同一个 UART 正在执行一次性接收时，本函数返回 BUSY。
 */
HW_UART_Status_e HW_UART_ReceiveRingBuffer_IT(UART_index_e UART_index,
                                              uint8_t *data,
                                              uint16_t *data_length);


#endif
