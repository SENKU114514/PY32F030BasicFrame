#ifndef LOWPOWER_KEY_INTERNAL_H
#define LOWPOWER_KEY_INTERNAL_H

#include <stdint.h>

/* 内部长按状态机：每 10ms 采样一次，连续按住 3 秒才确认。 */
#define LOWPOWER_KEY_SAMPLE_MS     10U
#define LOWPOWER_KEY_HOLD_MS     3000U

typedef struct
{
    uint16_t held_ms;       // 本次连续按住的时间
    uint8_t wait_release;   // 进入休眠时已按下，先等待松手
} LowPower_KeyState_t;

/* 建立按键状态，进入休眠时的按压不作为唤醒操作。 */
static void lowpower_key_reset(LowPower_KeyState_t *state, uint8_t pressed)
{
    state->held_ms = 0U;
    state->wait_release = pressed;
}

/* 每经过一个完整采样周期调用，返回 1 表示本次长按已确认。 */
static uint8_t lowpower_key_sample(LowPower_KeyState_t *state, uint8_t pressed)
{
    if (pressed == 0U)
    {
        state->held_ms = 0U;
        state->wait_release = 0U;
        return 0U;
    }
    if (state->wait_release != 0U)
        return 0U;
    if (state->held_ms < LOWPOWER_KEY_HOLD_MS)
        state->held_ms += LOWPOWER_KEY_SAMPLE_MS;
    return (state->held_ms >= LOWPOWER_KEY_HOLD_MS) ? 1U : 0U;
}

#endif
