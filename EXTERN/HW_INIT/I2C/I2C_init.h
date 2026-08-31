#ifndef __I2C_INIT_H__
#define __I2C_INIT_H__

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * I2C 硬件层：
 *
 * 1. 根据 SCL、SDA 引脚索引自动查找 GPIO 端口和复用功能；
 * 2. 初始化 PY32F030 的 I2C 主机；
 * 3. 提供带超时和状态返回值的轮询写、读、写后读接口。
 *
 * 当前只实现主机轮询模式，不包含中断、DMA 和从机模式。
 * 轮询接口会阻塞等待本次通信结束，不要在中断服务函数中调用。
 */


/*
 * I2C 逻辑编号。
 *
 * 芯片库已经把 I2C1 定义为外设地址宏，
 * 因此这里使用 I2C_BUS1，避免与芯片库名称冲突。
 */
typedef enum
{
    I2C_BUS1 = 0,
    I2C_BUS_COUNT,
} I2C_index_e;


/*
 * PY32F030 数据手册中允许复用为 I2C1_SCL 的引脚。
 * 实际使用前还要确认所选封装确实引出了该引脚。
 */
typedef enum
{
    SCL_A3 = 0,     /* PA3  -> I2C1_SCL，AF12 */
    SCL_A8,         /* PA8  -> I2C1_SCL，AF12 */
    SCL_A9,         /* PA9  -> I2C1_SCL，AF6  */
    SCL_A10,        /* PA10 -> I2C1_SCL，AF12 */
    SCL_A11,        /* PA11 -> I2C1_SCL，AF6  */
    SCL_B6,         /* PB6  -> I2C1_SCL，AF6  */
    SCL_B8,         /* PB8  -> I2C1_SCL，AF6  */
    SCL_F1,         /* PF1  -> I2C1_SCL，AF12；使用 HSE 时不能同时占用 */
    SCL_PIN_COUNT,
} I2C_SCL_Pin_e;


/*
 * PY32F030 数据手册中允许复用为 I2C1_SDA 的引脚。
 * 实际使用前还要确认所选封装确实引出了该引脚。
 */
typedef enum
{
    SDA_A2 = 0,     /* PA2  -> I2C1_SDA，AF12 */
    SDA_A7,         /* PA7  -> I2C1_SDA，AF12 */
    SDA_A9,         /* PA9  -> I2C1_SDA，AF12 */
    SDA_A10,        /* PA10 -> I2C1_SDA，AF6  */
    SDA_A12,        /* PA12 -> I2C1_SDA，AF6  */
    SDA_B7,         /* PB7  -> I2C1_SDA，AF6  */
    SDA_B8,         /* PB8  -> I2C1_SDA，AF12 */
    SDA_F0,         /* PF0  -> I2C1_SDA，AF12；使用 HSE 时不能同时占用 */
    SDA_PIN_COUNT,
} I2C_SDA_Pin_e;


/* I2C 操作状态，供 Service 层判断本次操作结果。 */
typedef enum
{
    HW_I2C_STATUS_OK = 0,
    HW_I2C_STATUS_INVALID_ARG,
    HW_I2C_STATUS_UNSUPPORTED,
    HW_I2C_STATUS_GPIO_INIT_FAILED,
    HW_I2C_STATUS_INIT_FAILED,
    HW_I2C_STATUS_NOT_READY,
    HW_I2C_STATUS_ALREADY_INITIALIZED,
    HW_I2C_STATUS_BUSY,
    HW_I2C_STATUS_TIMEOUT,
    HW_I2C_STATUS_NACK,
    HW_I2C_STATUS_BUS_ERROR,
    HW_I2C_STATUS_ARBITRATION_LOST,
    HW_I2C_STATUS_OVERRUN,
} HW_I2C_Status_e;


/*
 * 初始化 I2C 主机。
 *
 * 参数顺序：I2C 编号、SCL 引脚、SDA 引脚、总线速度。
 * ClockSpeed 当前只允许 100000U（标准模式）或 400000U（快速模式）。
 * I2C 的 SCL、SDA 必须在板上连接外部上拉电阻。
 * 同一个 I2C 只允许在启动阶段初始化一次；重复传入完全相同的配置会返回成功，
 * 已初始化后再更换引脚或速度会返回 HW_I2C_STATUS_ALREADY_INITIALIZED。
 */
HW_I2C_Status_e HW_I2C_init(I2C_index_e I2C_index,
                             I2C_SCL_Pin_e SCL_Pin,
                             I2C_SDA_Pin_e SDA_Pin,
                             uint32_t ClockSpeed);


/*
 * 向从机写入指定长度的数据。
 * device_address 直接填写 7 位地址，例如 0x68，不需要手动左移。
 */
HW_I2C_Status_e HW_I2C_Write(I2C_index_e I2C_index,
                              uint8_t device_address,
                              const uint8_t *data,
                              uint16_t data_length);


/*
 * 从从机读取指定长度的数据。
 * device_address 直接填写 7 位地址，例如 0x68，不需要手动左移。
 */
HW_I2C_Status_e HW_I2C_Read(I2C_index_e I2C_index,
                             uint8_t device_address,
                             uint8_t *data,
                             uint16_t data_length);


/*
 * 先写后读，中间使用重复起始信号，不发送停止信号。
 * 典型用途：先写入传感器寄存器地址，再读取该寄存器的数据。
 */
HW_I2C_Status_e HW_I2C_WriteRead(I2C_index_e I2C_index,
                                  uint8_t device_address,
                                  const uint8_t *write_data,
                                  uint16_t write_length,
                                  uint8_t *read_data,
                                  uint16_t read_length);


#ifdef __cplusplus
}
#endif

#endif
