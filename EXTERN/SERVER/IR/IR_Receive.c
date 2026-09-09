#include "IR_Receive.h"
#include "main.h"
#include <string.h>

/*
 * IR 接收使用示例：
 *
 * 1. 定义缓存并初始化，在打开 GPIO 下降沿中断之前执行。
 *    static IR_Receive_t ir;
 *    SERVER_IR_Receive_init(&ir);
 *
 * 2. 在选定引脚的下降沿回调中调用，now_us 由自己的计时接口提供。
 *    SERVER_IR_Receive_Edge(&ir, now_us);
 *    // 必须是连续的 32 位微秒时间，计时器选择、溢出扩展和 GPIO 由应用接入。
 *    // 当前 TIM 的 Stop/Start 返回单段耗时，不能直接当作这里的时间戳。
 *
 * 3. 在主循环或短周期任务中取出数据，具体按键动作由应用填写。
 *    IR_Receive_Frame_t frame;
 *    if (SERVER_IR_Receive_Get(&ir, &frame) == IR_RECEIVE_OK)
 *    {
 *        // frame.count 为 4，frame.bit_count 为 32。
 *        // frame.data[0] 是地址，frame.data[2] 是命令。
 *        // frame.data[1] 和 frame.data[3] 是对应反码，模块已校验。
 *    }
 *
 * 接收方式：仅记录相邻下降沿的间隔，不单独测量高低电平。
 * 正常引导段为 9ms + 4.5ms，数据 0 为引导段的 1/12，数据 1 为 1/6。
 * 位间隔允许正负 25% 偏差；引导段另设范围，避免把标称重复码当成正常帧。
 * 资料：Infineon AN2023-03，NEC Logical Define / Example message frame。
 * 注意：无法校验独立高低脉宽及末尾脉冲宽度；比例与反码校验不能排除所有噪声。
 */

/* 引导段允许范围，单位微秒，属于本模块的接收容差策略。 */
#define IR_NEC_LEADER_MIN_US       12000U
#define IR_NEC_LEADER_MAX_US       15000U
#define IR_NEC_TOLERANCE_PERCENT      25U

/* 缓存状态，一帧就绪后不再改写内容，等待主循环取走。 */
#define IR_STATE_WAIT                 0U
#define IR_STATE_RECEIVING            1U
#define IR_STATE_READY                2U

/* 初始化缓存，清除残帧和上次边沿时间。 */
void SERVER_IR_Receive_init(IR_Receive_t *receiver)
{
    if (receiver != NULL)
    {
        memset(receiver, 0, sizeof(*receiver));
    }
}

/* 接收下降沿时间戳，保存一帧需要的 32 个间隔。 */
void SERVER_IR_Receive_Edge(IR_Receive_t *receiver, uint32_t now_us)
{
    uint32_t interval;

    /* 第一步：保护待取走的数据，第一个边沿只建立计时起点。 */
    if ((receiver == NULL) || (receiver->state == IR_STATE_READY))
        return;
    if (receiver->has_edge == 0U)
    {
        receiver->last_us = now_us;
        receiver->has_edge = 1U;
        return;
    }
    interval = now_us - receiver->last_us;
    receiver->last_us = now_us;

    /* 第二步：遇到正常引导段就重新收帧，自动丢弃前面的残帧。 */
    if ((interval >= IR_NEC_LEADER_MIN_US) &&
        (interval <= IR_NEC_LEADER_MAX_US))
    {
        receiver->leader_us = (uint16_t)interval;
        receiver->count = 0U;
        receiver->state = IR_STATE_RECEIVING;
        return;
    }
    if (receiver->state != IR_STATE_RECEIVING)
        return;

    /* 第三步：明显异常或长时间无边沿时丢弃残帧，当前边沿成为新起点。 */
    if ((interval < 500U) || (interval > 4000U))
    {
        receiver->count = 0U;
        receiver->state = IR_STATE_WAIT;
        return;
    }

    /* 第四步：按顺序记录，末尾脉冲的下降沿补齐最后一位的间隔。 */
    receiver->interval_us[receiver->count++] = (uint16_t)interval;
    if (receiver->count == IR_RECEIVE_NEC_BITS)
        receiver->state = IR_STATE_READY;
}

/* 根据引导段和比例判断时间是否落在容差范围内，使用整数避免浮点计算。 */
static uint8_t ir_receive_match(uint16_t interval, uint16_t leader,
                                uint32_t divisor)
{
    uint32_t value = (uint32_t)interval * divisor * 100U;
    uint32_t lower = (uint32_t)leader * (100U - IR_NEC_TOLERANCE_PERCENT);
    uint32_t upper = (uint32_t)leader * (100U + IR_NEC_TOLERANCE_PERCENT);
    return ((value >= lower) && (value <= upper)) ? 1U : 0U;
}

/* 取出完整缓存，解码数据并检查地址和命令的反码。 */
IR_Receive_Status_e SERVER_IR_Receive_Get(IR_Receive_t *receiver,
                                        IR_Receive_Frame_t *frame)
{
    uint16_t intervals[IR_RECEIVE_NEC_BITS];
    uint16_t leader;
    uint32_t primask;
    uint32_t i;
    IR_Receive_Frame_t result = {0};

    /* 第一步：清空本次输出，未收到完整帧时不返回残留数据。 */
    if (frame == NULL)
        return IR_RECEIVE_INVALID_ARG;
    memset(frame, 0, sizeof(*frame));
    if (receiver == NULL)
        return IR_RECEIVE_INVALID_ARG;

    /* 第二步：短暂保护缓存复制，取走后释放接收区，再恢复原中断状态。 */
    primask = __get_PRIMASK();
    __disable_irq();
    if (receiver->state != IR_STATE_READY)
    {
        __set_PRIMASK(primask);
        return IR_RECEIVE_EMPTY;
    }
    memcpy(intervals, receiver->interval_us, sizeof(intervals));
    leader = receiver->leader_us;
    receiver->state = IR_STATE_WAIT;
    receiver->count = 0U;
    receiver->has_edge = 0U;
    __set_PRIMASK(primask);

    /* 第三步：按实测引导段计算 0/1 比例，按低位在先组成四个字节。 */
    for (i = 0U; i < IR_RECEIVE_NEC_BITS; i++)
    {
        if (ir_receive_match(intervals[i], leader, 12U))
            continue;
        if (!ir_receive_match(intervals[i], leader, 6U))
            return IR_RECEIVE_TIMING_ERROR;
        result.data[i / 8U] |= (uint8_t)(1U << (i % 8U));
    }

    /* 第四步：校验两个反码，只有全部通过才交出有效数组和数量。 */
    if (((uint8_t)(result.data[0] ^ result.data[1]) != 0xFFU) ||
        ((uint8_t)(result.data[2] ^ result.data[3]) != 0xFFU))
        return IR_RECEIVE_CHECK_ERROR;
    result.count = IR_RECEIVE_NEC_BYTES;
    result.bit_count = IR_RECEIVE_NEC_BITS;
    result.leader_us = leader;
    *frame = result;
    return IR_RECEIVE_OK;
}
