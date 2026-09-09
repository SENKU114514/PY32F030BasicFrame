#ifndef __IR_RECEIVE_H__
#define __IR_RECEIVE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 标准 NEC 一帧包含 32 位，按低位在先的顺序组成 4 个字节。 */
#define IR_RECEIVE_NEC_BITS          32U
#define IR_RECEIVE_NEC_BYTES         4U

/* 接收状态，只有 OK 表示本次取到了校验通过的数据。 */
typedef enum
{
    IR_RECEIVE_OK = 0,
    IR_RECEIVE_EMPTY,
    IR_RECEIVE_INVALID_ARG,
    IR_RECEIVE_TIMING_ERROR,
    IR_RECEIVE_CHECK_ERROR,
} IR_Receive_Status_e;

/* 一帧有效数据，data 依次为地址、地址反码、命令、命令反码。 */
typedef struct
{
    uint8_t data[IR_RECEIVE_NEC_BYTES]; // 校验通过后输出的字节数组
    uint8_t count;                    // 有效字节数量，标准 NEC 为 4
    uint8_t bit_count;                // 有效位数量，标准 NEC 为 32
    uint16_t leader_us;               // 本帧引导段的实测总时间，单位微秒
} IR_Receive_Frame_t;

/* 接收缓存，由模块内部维护，使用者只需定义一个实例。 */
typedef struct
{
    uint32_t last_us;                            // 上一次下降沿的时间戳
    uint16_t interval_us[IR_RECEIVE_NEC_BITS];    // 每一位的低电平加高电平时间
    uint16_t leader_us;                          // 引导段时间，用于自动计算比例
    uint8_t count;                               // 已记录的时间间隔数量
    uint8_t has_edge;                            // 是否已经收到计时起点
    uint8_t state;                               // 等待引导、接收中、一帧就绪
} IR_Receive_t;

/* 初始化 NEC 接收缓存，在开启 GPIO 中断之前调用。 */
void SERVER_IR_Receive_init(IR_Receive_t *receiver);

/* 每次下降沿传入连续的微秒时间戳，只记录间隔，不执行解码。 */
/*
 * 1. now_us 来自外部连续计时源，不是毫秒 tick，也不是直接读取的 16 位 CNT。
 * 2. 允许 uint32_t 自然回绕，相邻调用间隔必须小于一个完整回绕周期。
 * 3. 同一实例只允许一个 GPIO 中断入口写入，不能嵌套调用。
 * 4. 单帧缓存就绪后丢弃新边沿，主循环取走后再接收下一帧。
 * 5. 只支持标准 NEC 正常帧，不输出长按重复帧，不支持扩展 NEC 地址。
 */
void SERVER_IR_Receive_Edge(IR_Receive_t *receiver, uint32_t now_us);

/* 主循环取出一帧并完成比例解码和反码校验，没有有效数据时 count 为 0。 */
/* 取帧内部短暂关闭中断复制缓存，恢复原中断状态后再解码。 */
IR_Receive_Status_e SERVER_IR_Receive_Get(IR_Receive_t *receiver,
                                        IR_Receive_Frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif
