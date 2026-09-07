#ifndef IR_RECEIVE_TEST_PLATFORM_H
#define IR_RECEIVE_TEST_PLATFORM_H
#include <stdint.h>
extern uint32_t test_primask;
static inline uint32_t __get_PRIMASK(void) { return test_primask; }
static inline void __disable_irq(void) { test_primask = 1U; }
static inline void __set_PRIMASK(uint32_t value) { test_primask = value; }
#endif
