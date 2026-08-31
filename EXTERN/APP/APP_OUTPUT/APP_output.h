#ifndef __APP_OUTPUT_H__
#define __APP_OUTPUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
    APP_OUTPUT_OK,
    APP_OUTPUT_FAIL,
} APP_OUTPUT_State_e;

/* 输出引脚定义 */
#define LED1   A3
#define LED2   A4


/* 输出业务初始化 */
APP_OUTPUT_State_e APP_Output_init(void);
APP_OUTPUT_State_e APP_Output_LED1(uint8_t state);
APP_OUTPUT_State_e APP_Output_LED2(uint8_t state);

#ifdef __cplusplus
}
#endif

#endif
