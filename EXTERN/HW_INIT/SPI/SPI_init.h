#ifndef __SPI_INIT_H__
#define __SPI_INIT_H__

#include "main.h"

extern volatile uint8_t SPI_DMA_Flag;

/*
 * SPI 逻辑编号。
 * SPI1 已被芯片库定义为外设地址宏，因此这里使用 SPI_BUS1。
 */
typedef enum
{
	SPI_BUS1 = 0,
	SPI_BUS_COUNT,
}SPI_index_e;

/* PY32F030 数据手册中允许复用为 SPI1_SCK 的引脚。 */
typedef enum
{
	SCK_A1 = 0,     /* PA1 -> SPI1_SCK，AF0  */
	SCK_A2,         /* PA2 -> SPI1_SCK，AF10 */
	SCK_A5,         /* PA5 -> SPI1_SCK，AF0  */
	SCK_A9,         /* PA9 -> SPI1_SCK，AF10 */
	SCK_B3,         /* PB3 -> SPI1_SCK，AF0  */
	SCK_PIN_COUNT,
}SPI_SCK_Pin_e;

/*
 * PY32F030 数据手册中允许复用为 SPI1_MOSI 的引脚。
 * MOSI_NULL 用于没有 MOSI 的半双工接收预留；当前文件暂未提供独立接收函数。
 */
typedef enum
{
	MOSI_NULL = 0,
	MOSI_A1,        /* PA1  -> SPI1_MOSI，AF10 */
	MOSI_A2,        /* PA2  -> SPI1_MOSI，AF0  */
	MOSI_A3,        /* PA3  -> SPI1_MOSI，AF10 */
	MOSI_A7,        /* PA7  -> SPI1_MOSI，AF0  */
	MOSI_A8,        /* PA8  -> SPI1_MOSI，AF10 */
	MOSI_A12,       /* PA12 -> SPI1_MOSI，AF0  */
	MOSI_B5,        /* PB5  -> SPI1_MOSI，AF0  */
	MOSI_PIN_COUNT,
}SPI_MOSI_Pin_e;

/* PY32F030 数据手册中允许复用为 SPI1_MISO 的引脚。 */
typedef enum
{
	MISO_NULL = 0,  /* 不使用 MISO，配置为半双工发送 */
	MISO_A0,        /* PA0  -> SPI1_MISO，AF10 */
	MISO_A6,        /* PA6  -> SPI1_MISO，AF0  */
	MISO_A7,        /* PA7  -> SPI1_MISO，AF10 */
	MISO_A11,       /* PA11 -> SPI1_MISO，AF0  */
	MISO_A13,       /* PA13 -> SPI1_MISO，AF10；会占用 SWDIO */
	MISO_B4,        /* PB4  -> SPI1_MISO，AF0  */
	MISO_PIN_COUNT,
}SPI_MISO_Pin_e;

/* SPI 硬件层操作状态。 */
typedef enum
{
	HW_SPI_STATUS_OK = 0,
	HW_SPI_STATUS_INVALID_ARG,
	HW_SPI_STATUS_UNSUPPORTED,
	HW_SPI_STATUS_GPIO_INIT_FAILED,
	HW_SPI_STATUS_DMA_INIT_FAILED,
	HW_SPI_STATUS_INIT_FAILED,
	HW_SPI_STATUS_NOT_READY,
	HW_SPI_STATUS_ALREADY_INITIALIZED,
	HW_SPI_STATUS_NO_TRANSFER,
	HW_SPI_STATUS_BUSY,
	HW_SPI_STATUS_TIMEOUT,
	HW_SPI_STATUS_DMA_ERROR,
}HW_SPI_Status_e;

/*
 * 初始化 SPI：参数顺序为 SPI 编号、SCK、MOSI、MISO。
 * MISO_NULL 表示只发送的半双工模式；MOSI_NULL 为未来独立接收模式预留。
 */
HW_SPI_Status_e HW_SPI_DMA_init(SPI_index_e spi_index,
								SPI_SCK_Pin_e SCK_Pin,
								SPI_MOSI_Pin_e MOSI_Pin,
								SPI_MISO_Pin_e MISO_Pin);
HW_SPI_Status_e SPI_TransmitReceive_DMA(uint8_t *pTxData, uint8_t *pRxData, uint16_t Size);//SPI收发函数
HW_SPI_Status_e SPI_Transmit_DMA(uint8_t *pTxData, uint16_t Size);//SPI发送函数
HW_SPI_Status_e SPI_WaitAndCheckEndOfTransfer(void);

/*回调弱定义函数__weak void SPI_DmaTxIRQCallback(void)*/

#endif


