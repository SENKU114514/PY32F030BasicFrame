#ifndef __GPIO_TYPES_H__
#define __GPIO_TYPES_H__

/*
 * GPIO public shared types.
 * Keep only types and constants that are part of the GPIO module API here.
 * Private GPIO mapping tables stay in GPIO_init.c.
 */
#include "py32f0xx_ll_gpio.h"

/* GPIO index definitions, generated for each port available on the target MCU. */
#define GPIO_INDEX_PORT(P) \
    P##0,                  \
    P##1,                  \
    P##2,                  \
    P##3,                  \
    P##4,                  \
    P##5,                  \
    P##6,                  \
    P##7,                  \
    P##8,                  \
    P##9,                  \
    P##10,                 \
    P##11,                 \
    P##12,                 \
    P##13,                 \
    P##14,                 \
    P##15

typedef enum
{
#ifdef GPIOA
    GPIO_INDEX_PORT(A),
#endif

#ifdef GPIOB
    GPIO_INDEX_PORT(B),
#endif

#ifdef GPIOC
    GPIO_INDEX_PORT(C),
#endif

#ifdef GPIOF
    GPIO_INDEX_PORT(F),
#endif
} GPIO_index_e;

#undef GPIO_INDEX_PORT


#endif /* __GPIO_TYPES_H__ */
