#include "LowPower_key.h"
#include <assert.h>
#include <stdio.h>

/* 验证 3 秒门槛、短按复位和进入时按键已按下的行为。 */
int main(void)
{
    LowPower_KeyState_t state;
    unsigned i;

    lowpower_key_reset(&state, 0U);
    for (i = 0; i < 299; i++)
        assert(lowpower_key_sample(&state, 1U) == 0U);
    assert(state.held_ms == 2990U);
    assert(lowpower_key_sample(&state, 1U) == 1U);
    assert(state.held_ms == 3000U);

    /* 松开后必须重新累计，多个短按不能拼成一次长按。 */
    assert(lowpower_key_sample(&state, 0U) == 0U);
    assert(state.held_ms == 0U);
    for (i = 0; i < 200; i++)
        assert(lowpower_key_sample(&state, 1U) == 0U);
    assert(lowpower_key_sample(&state, 0U) == 0U);
    for (i = 0; i < 299; i++)
        assert(lowpower_key_sample(&state, 1U) == 0U);
    assert(lowpower_key_sample(&state, 1U) == 1U);

    /* 进入时已经按下，即使继续按十秒也不能唤醒，须先松开。 */
    lowpower_key_reset(&state, 1U);
    for (i = 0; i < 1000; i++)
        assert(lowpower_key_sample(&state, 1U) == 0U);
    assert(state.held_ms == 0U);
    assert(lowpower_key_sample(&state, 0U) == 0U);
    for (i = 0; i < 299; i++)
        assert(lowpower_key_sample(&state, 1U) == 0U);
    assert(lowpower_key_sample(&state, 1U) == 1U);
    puts("LowPower key tests passed");
    return 0;
}
