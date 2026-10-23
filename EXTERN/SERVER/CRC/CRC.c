#include "SERVER/CRC/CRC.h"

/* 对指定字节序列逐字节异或，计算通用 8 位异或校验值。 */
uint8_t CRC_XOR(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0U;
    uint16_t index;

    if (data == NULL)
    {
        return 0U;
    }

    for (index = 0U; index < len; index++)
    {
        crc ^= data[index];
    }

    return crc;
}


//CRC8
uint8_t CRC_8(const uint8_t *data, uint8_t len){
	uint8_t crc = 0U;
	for (uint8_t i = 0U; i < len; i++)
	{
			crc ^= data[i];
			for (uint8_t j = 0U; j < 8U; j++)
			{
					if (crc & 0x80U)
					{
							crc = (crc << 1U) ^ 0x07U;
					}
					else
					{
							crc <<= 1U;
					}
			}
	}
	return crc;
}

//CRC16
uint16_t CRC16_MODBUS(const uint8_t *data, uint16_t len){
    uint16_t crc = 0xFFFFU;
    uint16_t i, j;

    for (i = 0U; i < len; i++)
    {
        crc ^= (uint16_t)data[i];
        for (j = 0U; j < 8U; j++)
        {
            if (crc & 0x0001U)
            {
                crc >>= 1U;
                crc ^= 0xA001U; // 0x8005 位反转多项式
            }
            else
            {
                crc >>= 1U;
            }
        }
    }
    return crc;
}

