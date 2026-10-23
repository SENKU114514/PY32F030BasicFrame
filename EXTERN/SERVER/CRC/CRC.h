#ifndef __SERVER_CRC_H__
#define __SERVER_CRC_H__

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t CRC_8(const uint8_t *data, uint8_t len);//CRC8校验
uint16_t CRC16_MODBUS(const uint8_t *data, uint16_t len);//CRC16_MODBUS校验
uint8_t CRC_XOR(const uint8_t *data, uint16_t len);//异或校验

#ifdef __cplusplus
}
#endif

#endif

