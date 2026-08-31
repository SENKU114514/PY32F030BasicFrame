#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif


/*code*/
#include "main.h"

#define UARTx UART2
#define TX_pin TX_A4
#define RX_pin RX_A5

#define LOG_BUFFER_SIZE    128U

typedef enum{
	LOG_OK,
	LOG_FAIL,
}Log_State_e;

Log_State_e Log_init(void);

#if LOG_ENABLE

#define LOG_DEBUG(format,...)                                      \
     do                                                              \
    {                                                               \
        char log_buffer[LOG_BUFFER_SIZE];                           \
        int log_length;                                             \
        uint16_t send_length;                                       \
                                                                    \
        log_length = snprintf(log_buffer,                           \
                              sizeof(log_buffer),                   \
                              (format),                             \
                              ##__VA_ARGS__);                       \
                                                                    \
        if (log_length > 0)                                         \
        {                                                           \
            if (log_length >= (int)sizeof(log_buffer))              \
            {                                                       \
                send_length = (uint16_t)(sizeof(log_buffer) - 1U);  \
            }                                                       \
            else                                                    \
            {                                                       \
                send_length = (uint16_t)log_length;                 \
            }                                                       \
                                                                    \
            (void)HW_UART_Send(                                     \
                UARTx,                                              \
                (const uint8_t *)log_buffer,                        \
                send_length);                                       \
        }                                                           \
    } while (0)

#else

#define LOG_DEBUG(...)    do { } while (0)

#endif



#ifdef __cplusplus
}
#endif

#endif


