#include "IR_Receive.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uint32_t test_primask;

/* 模拟下降沿序列，可设置引导段缩放、异常位和起点回绕。 */
static void send_frame(IR_Receive_t *ir, const uint8_t *data,
                       uint32_t start, uint32_t leader, int bad_bit)
{
    unsigned i;
    SERVER_IR_Receive_Edge(ir, start);
    start += leader;
    SERVER_IR_Receive_Edge(ir, start);
    for (i = 0; i < 32; i++)
    {
        uint32_t interval = (data[i / 8] & (1U << (i % 8))) ? leader / 6 : leader / 12;
        if ((int)i == bad_bit) interval = 1600;
        start += interval;
        SERVER_IR_Receive_Edge(ir, start);
    }
}

int main(void)
{
    IR_Receive_t ir;
    IR_Receive_Frame_t frame;
    const uint8_t data[] = {0x12, 0xED, 0x34, 0xCB};
    const uint8_t bad[] = {0x12, 0xEC, 0x34, 0xCB};
    unsigned scale;

    /* 检查正常帧、比例自适应、自然回绕和有效数量。 */
    for (scale = 12000; scale <= 15000; scale += 1500)
    {
        SERVER_IR_Receive_init(&ir);
        send_frame(&ir, data, UINT32_MAX - 5000U, scale, -1);
        assert(SERVER_IR_Receive_Get(&ir, &frame) == IR_RECEIVE_OK);
        assert(frame.count == 4 && frame.bit_count == 32);
        assert(memcmp(frame.data, data, 4) == 0);
        assert(frame.leader_us == scale);
        assert(SERVER_IR_Receive_Get(&ir, &frame) == IR_RECEIVE_EMPTY);
        assert(frame.count == 0);
    }

    /* 实测位间隔存在正负 20% 偏差时仍可解码。 */
    for (scale = 80; scale <= 120; scale += 40)
    {
        unsigned i;
        send_frame(&ir, data, 100000, 13500, -1);
        for (i = 0; i < 32; i++)
            ir.interval_us[i] = (uint16_t)(ir.interval_us[i] * scale / 100U);
        assert(SERVER_IR_Receive_Get(&ir, &frame) == IR_RECEIVE_OK);
        assert(memcmp(frame.data, data, 4) == 0);
    }

    /* 校验失败和非法比例不得返回有效数据。 */
    send_frame(&ir, bad, 100000, 13500, -1);
    assert(SERVER_IR_Receive_Get(&ir, &frame) == IR_RECEIVE_CHECK_ERROR);
    assert(frame.count == 0);
    send_frame(&ir, data, 200000, 13500, 5);
    assert(SERVER_IR_Receive_Get(&ir, &frame) == IR_RECEIVE_TIMING_ERROR);
    assert(frame.count == 0);

    /* 残帧、超时和重复码后，下一完整正常帧可以重新同步。 */
    SERVER_IR_Receive_Edge(&ir, 300000);
    SERVER_IR_Receive_Edge(&ir, 313500);
    SERVER_IR_Receive_Edge(&ir, 314625);
    SERVER_IR_Receive_Edge(&ir, 350000);
    SERVER_IR_Receive_Edge(&ir, 361250);
    SERVER_IR_Receive_Edge(&ir, 362000);
    assert(SERVER_IR_Receive_Get(&ir, &frame) == IR_RECEIVE_EMPTY);
    send_frame(&ir, data, 400000, 13500, -1);

    /* 缓存满后不覆盖，取帧保留调用前的中断状态。 */
    send_frame(&ir, bad, 500000, 13500, -1);
    test_primask = 1;
    assert(SERVER_IR_Receive_Get(&ir, &frame) == IR_RECEIVE_OK);
    assert(test_primask == 1);
    assert(memcmp(frame.data, data, 4) == 0);
    test_primask = 0;
    assert(SERVER_IR_Receive_Get(&ir, &frame) == IR_RECEIVE_EMPTY);
    assert(test_primask == 0);
    assert(SERVER_IR_Receive_Get(NULL, &frame) == IR_RECEIVE_INVALID_ARG);
    assert(frame.count == 0);
    assert(SERVER_IR_Receive_Get(&ir, NULL) == IR_RECEIVE_INVALID_ARG);
    puts("IR receive tests passed");
    return 0;
}
