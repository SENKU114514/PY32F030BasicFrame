#ifndef __SYSTEM_INIT_H__
#define __SYSTEM_INIT_H__

#include "main.h"

/* Configure before initializing peripherals or MAG.
 * PY32F002B: 4/8/24 MHz; legacy: 4/8/16/24/48 MHz.
 * Unsupported SYS_ClockConfig requests leave hardware unchanged.
 */
typedef enum { SYS_CLOCK_OK = 0, SYS_CLOCK_UNSUPPORTED } SYS_ClockStatus_e;
SYS_ClockStatus_e SYS_ClockConfig(uint8_t frequency_mhz);

void APP_SystemClockConfig(uint8_t FRE);//系统时钟初始化

#endif


