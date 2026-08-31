#ifndef __APP_COMM_H__
#define __APP_COMM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
    APP_COMM_OK,
    APP_COMM_FAIL,
} APP_COMM_State_e;

/* 通信引脚定义。 */
#define Tuya_Tx    A0
#define Tuya_Rx    A1

#define DC2431_Tx  A2
#define DC2431_Rx  A3

/* 通信业务初始化。 */
APP_COMM_State_e APP_Comm_init(void);

#ifdef __cplusplus
}
#endif

#endif
