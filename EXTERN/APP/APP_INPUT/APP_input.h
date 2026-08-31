#ifndef __APP_INPUT_H__
#define __APP_INPUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
    APP_INPUT_OK,
    APP_INPUT_ZERO_AC_FAIL,
    APP_INPUT_FAIL
} APP_INPUT_State_e;

/* 输入引脚定义 */
//#define ZERO_AC     A5
//#define RST         B2
//#define DC2431OUT   A4

/* 输入业务初始化 */
APP_INPUT_State_e APP_Input_init(void);

void APP_INPUT_KEY1(void);


#ifdef __cplusplus
}
#endif

#endif
