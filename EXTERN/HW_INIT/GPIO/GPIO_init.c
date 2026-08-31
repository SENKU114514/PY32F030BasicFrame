#include "./HW_INIT/GPIO/GPIO_init.h"

#include <stddef.h>
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_exti.h"
#include "./SERVER/LOG/LOG.h"

//拉高引脚：LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_11);
//拉低引脚：LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_11);
//翻转引脚：LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_11);


/* GPIO pin map is private to the GPIO hardware implementation. */
typedef struct
{
	GPIO_TypeDef *gpio_port;
	uint32_t gpio_pin;
} GPIO_GPIO_MAP_t;

#define GPIO_MAP_PORT(P)                         \
    [P##0 ] = {GPIO##P, LL_GPIO_PIN_0 },         \
    [P##1 ] = {GPIO##P, LL_GPIO_PIN_1 },         \
    [P##2 ] = {GPIO##P, LL_GPIO_PIN_2 },         \
    [P##3 ] = {GPIO##P, LL_GPIO_PIN_3 },         \
    [P##4 ] = {GPIO##P, LL_GPIO_PIN_4 },         \
    [P##5 ] = {GPIO##P, LL_GPIO_PIN_5 },         \
    [P##6 ] = {GPIO##P, LL_GPIO_PIN_6 },         \
    [P##7 ] = {GPIO##P, LL_GPIO_PIN_7 },         \
    [P##8 ] = {GPIO##P, LL_GPIO_PIN_8 },         \
    [P##9 ] = {GPIO##P, LL_GPIO_PIN_9 },         \
    [P##10] = {GPIO##P, LL_GPIO_PIN_10},         \
    [P##11] = {GPIO##P, LL_GPIO_PIN_11},         \
    [P##12] = {GPIO##P, LL_GPIO_PIN_12},         \
    [P##13] = {GPIO##P, LL_GPIO_PIN_13},         \
    [P##14] = {GPIO##P, LL_GPIO_PIN_14},         \
    [P##15] = {GPIO##P, LL_GPIO_PIN_15}


 
const GPIO_GPIO_MAP_t GPIO_GPIO_MAP[] = {
	#ifdef GPIOA
    GPIO_MAP_PORT(A),
#endif

#ifdef GPIOB
    GPIO_MAP_PORT(B),
#endif

#ifdef GPIOC
    GPIO_MAP_PORT(C),
#endif

#ifdef GPIOF
    GPIO_MAP_PORT(F),
#endif
};

#undef GPIO_MAP_PORT  //取消宏定义，防止污染

//计算元素个数
#define GPIO_MAP_COUNT \
    (sizeof(GPIO_GPIO_MAP) / sizeof(GPIO_GPIO_MAP[0]))

/*
 * GPIO 到 EXTI 的内部映射表。
 *
 * 当前 PY32F030x8 的 LL 库只提供 EXTI0~8 的端口来源选择，
 * 因此本表只收录引脚号 0~8。调用方只传 GPIO 索引，不需要填写
 * EXTI Line、EXTI 端口或 NVIC IRQ。
 */
typedef struct
{
    GPIO_index_e gpio_index;
    uint32_t exti_port;
    uint32_t exti_config_line;
    uint32_t exti_line;
    uint8_t line_index;
    IRQn_Type irqn;
} GPIO_EXTI_MAP_t;

#define GPIO_EXTI_MAP(GPIO_INDEX, EXTI_PORT, EXTI_CONFIG_LINE, EXTI_LINE, LINE_INDEX, IRQ) \
    {GPIO_INDEX, EXTI_PORT, EXTI_CONFIG_LINE, EXTI_LINE, LINE_INDEX, IRQ}

static const GPIO_EXTI_MAP_t GPIO_EXTI_MAP[] =
{
#ifdef GPIOA
    GPIO_EXTI_MAP(A0, LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE0, LL_EXTI_LINE_0, 0U, EXTI0_1_IRQn),
    GPIO_EXTI_MAP(A1, LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE1, LL_EXTI_LINE_1, 1U, EXTI0_1_IRQn),
    GPIO_EXTI_MAP(A2, LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE2, LL_EXTI_LINE_2, 2U, EXTI2_3_IRQn),
    GPIO_EXTI_MAP(A3, LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE3, LL_EXTI_LINE_3, 3U, EXTI2_3_IRQn),
    GPIO_EXTI_MAP(A4, LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE4, LL_EXTI_LINE_4, 4U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(A5, LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE5, LL_EXTI_LINE_5, 5U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(A6, LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE6, LL_EXTI_LINE_6, 6U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(A7, LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE7, LL_EXTI_LINE_7, 7U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(A8, LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE8, LL_EXTI_LINE_8, 8U, EXTI4_15_IRQn),
#endif
#ifdef GPIOB
    GPIO_EXTI_MAP(B0, LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE0, LL_EXTI_LINE_0, 0U, EXTI0_1_IRQn),
    GPIO_EXTI_MAP(B1, LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE1, LL_EXTI_LINE_1, 1U, EXTI0_1_IRQn),
    GPIO_EXTI_MAP(B2, LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE2, LL_EXTI_LINE_2, 2U, EXTI2_3_IRQn),
    GPIO_EXTI_MAP(B3, LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE3, LL_EXTI_LINE_3, 3U, EXTI2_3_IRQn),
    GPIO_EXTI_MAP(B4, LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE4, LL_EXTI_LINE_4, 4U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(B5, LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE5, LL_EXTI_LINE_5, 5U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(B6, LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE6, LL_EXTI_LINE_6, 6U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(B7, LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE7, LL_EXTI_LINE_7, 7U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(B8, LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE8, LL_EXTI_LINE_8, 8U, EXTI4_15_IRQn),
#endif
#ifdef GPIOF
    GPIO_EXTI_MAP(F0, LL_EXTI_CONFIG_PORTF, LL_EXTI_CONFIG_LINE0, LL_EXTI_LINE_0, 0U, EXTI0_1_IRQn),
    GPIO_EXTI_MAP(F1, LL_EXTI_CONFIG_PORTF, LL_EXTI_CONFIG_LINE1, LL_EXTI_LINE_1, 1U, EXTI0_1_IRQn),
    GPIO_EXTI_MAP(F2, LL_EXTI_CONFIG_PORTF, LL_EXTI_CONFIG_LINE2, LL_EXTI_LINE_2, 2U, EXTI2_3_IRQn),
    GPIO_EXTI_MAP(F3, LL_EXTI_CONFIG_PORTF, LL_EXTI_CONFIG_LINE3, LL_EXTI_LINE_3, 3U, EXTI2_3_IRQn),
    GPIO_EXTI_MAP(F4, LL_EXTI_CONFIG_PORTF, LL_EXTI_CONFIG_LINE4, LL_EXTI_LINE_4, 4U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(F5, LL_EXTI_CONFIG_PORTF, LL_EXTI_CONFIG_LINE5, LL_EXTI_LINE_5, 5U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(F6, LL_EXTI_CONFIG_PORTF, LL_EXTI_CONFIG_LINE6, LL_EXTI_LINE_6, 6U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(F7, LL_EXTI_CONFIG_PORTF, LL_EXTI_CONFIG_LINE7, LL_EXTI_LINE_7, 7U, EXTI4_15_IRQn),
    GPIO_EXTI_MAP(F8, LL_EXTI_CONFIG_PORTF, LL_EXTI_CONFIG_LINE8, LL_EXTI_LINE_8, 8U, EXTI4_15_IRQn),
#endif
};

#undef GPIO_EXTI_MAP

#define GPIO_EXTI_MAP_COUNT \
    (sizeof(GPIO_EXTI_MAP) / sizeof(GPIO_EXTI_MAP[0]))

/* 每条 EXTI0~8 当前登记的引脚；NULL 表示该通道尚未由本模块使用。 */
static const GPIO_EXTI_MAP_t *s_gpio_exti_line_map[9] = {NULL};
			
/*使能GPIO*/
static HW_GPIO_Status_e HW_GPIO_EnableClock(GPIO_TypeDef *gpio_port){
    if (gpio_port == NULL)
    {
        return HW_GPIO_STATUS_INVALID_ARG;
    }

#ifdef GPIOA
    if (gpio_port == GPIOA)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
        return HW_GPIO_STATUS_OK;
    }
#endif

#ifdef GPIOB
    if (gpio_port == GPIOB)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
        return HW_GPIO_STATUS_OK;
    }
#endif

#ifdef GPIOC
    if (gpio_port == GPIOC)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
        return HW_STATUS_OK;
    }
#endif

#ifdef GPIOF
    if (gpio_port == GPIOF)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);
        return HW_GPIO_STATUS_OK;
    }
#endif

    return HW_GPIO_STATUS_UNSUPPORTED;
}

/* 从 GPIO 中断映射表中查找指定引脚的 EXTI 配置。 */
static const GPIO_EXTI_MAP_t *HW_GPIO_FindEXTIMap(GPIO_index_e GPIO_index)
{
    uint32_t index;

    for (index = 0U; index < GPIO_EXTI_MAP_COUNT; index++)
    {
        if (GPIO_EXTI_MAP[index].gpio_index == GPIO_index)
        {
            return &GPIO_EXTI_MAP[index];
        }
    }

    return NULL;
}

/* 按选择的边沿打开 EXTI 触发；无效枚举返回参数错误。 */
static HW_GPIO_Status_e HW_GPIO_ConfigEXTITrigger(
    uint32_t exti_line,
    GPIO_IT_Trigger_e trigger)
{
    LL_EXTI_DisableRisingTrig(exti_line);
    LL_EXTI_DisableFallingTrig(exti_line);

    switch (trigger)
    {
        case GPIO_IT_RISING:
            LL_EXTI_EnableRisingTrig(exti_line);
            break;

        case GPIO_IT_FALLING:
            LL_EXTI_EnableFallingTrig(exti_line);
            break;

        case GPIO_IT_RISING_FALLING:
            LL_EXTI_EnableRisingTrig(exti_line);
            LL_EXTI_EnableFallingTrig(exti_line);
            break;

        default:
            return HW_GPIO_STATUS_INVALID_ARG;
    }

    return HW_GPIO_STATUS_OK;
}

/* 初始化 GPIO 外部中断输入，调用方只需要传 GPIO 引脚和触发边沿。 */
HW_GPIO_Status_e HW_GPIO_INPUT_IT_init(
    GPIO_index_e GPIO_index,
		GPIO_pull_e pull,
    GPIO_IT_Trigger_e trigger){
    const GPIO_EXTI_MAP_t *exti_cfg;
    const GPIO_GPIO_MAP_t *gpio_cfg;
    LL_GPIO_InitTypeDef gpio_init = {0};
    HW_GPIO_Status_e status;

    if ((uint32_t)GPIO_index >= GPIO_MAP_COUNT)
    {
        return HW_GPIO_STATUS_INVALID_ARG;
    }

    exti_cfg = HW_GPIO_FindEXTIMap(GPIO_index);
    if (exti_cfg == NULL)
    {
        return HW_GPIO_STATUS_UNSUPPORTED;
    }

    /* 同一 EXTI Line 只能从 A/B/F 的同号引脚中选择一个来源。 */
    if ((s_gpio_exti_line_map[exti_cfg->line_index] != NULL) &&
        (s_gpio_exti_line_map[exti_cfg->line_index] != exti_cfg))
    {
        return HW_GPIO_STATUS_IT_CONFLICT;
    }

    gpio_cfg = &GPIO_GPIO_MAP[GPIO_index];
    if (gpio_cfg->gpio_port == NULL)
    {
        return HW_GPIO_STATUS_UNSUPPORTED;
    }

    status = HW_GPIO_EnableClock(gpio_cfg->gpio_port);
    if (status != HW_GPIO_STATUS_OK)
    {
        return status;
    }

    /* 两参数版本固定无上下拉，避免默认把输入偏置到错误电平。 */
    gpio_init.Pin = gpio_cfg->gpio_pin;
    gpio_init.Mode = LL_GPIO_MODE_INPUT;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_LOW;
		
    /*自定义上下拉*/
		switch(pull){
			case NO:gpio_init.Pull = LL_GPIO_PULL_NO;break;
			case UP:gpio_init.Pull = LL_GPIO_PULL_UP;break;
			case DOWN:gpio_init.Pull = LL_GPIO_PULL_DOWN;break;
			default:gpio_init.Pull = LL_GPIO_PULL_NO;break;
		}
		
    if (LL_GPIO_Init(gpio_cfg->gpio_port, &gpio_init) != SUCCESS)
    {
        return HW_GPIO_STATUS_INIT_FAILED;
    }

    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
    LL_EXTI_SetEXTISource(exti_cfg->exti_port, exti_cfg->exti_config_line);

    status = HW_GPIO_ConfigEXTITrigger(exti_cfg->exti_line, trigger);
    if (status != HW_GPIO_STATUS_OK)
    {
        return status;
    }

    LL_EXTI_ClearFlag(exti_cfg->exti_line);
    LL_EXTI_EnableIT(exti_cfg->exti_line);

    NVIC_SetPriority(exti_cfg->irqn, 0U);
    NVIC_EnableIRQ(exti_cfg->irqn);

    s_gpio_exti_line_map[exti_cfg->line_index] = exti_cfg;

    return HW_GPIO_STATUS_OK;
}

/* 处理一个共享 EXTI IRQ 内的已登记中断线。 */
static void HW_GPIO_EXTI_IRQHandler(uint8_t first_line, uint8_t last_line)
{
    uint8_t line_index;
    const GPIO_EXTI_MAP_t *exti_cfg;

    for (line_index = first_line; line_index <= last_line; line_index++)
    {
        exti_cfg = s_gpio_exti_line_map[line_index];
        if ((exti_cfg != NULL) &&
            (LL_EXTI_IsActiveFlag(exti_cfg->exti_line) != 0U))
        {
            /* 先清标志，再进入 APP 回调，避免回调期间重复进入同一中断。 */
            LL_EXTI_ClearFlag(exti_cfg->exti_line);
            HW_GPIO_INPUT_IT_Callback(exti_cfg->gpio_index);
        }
    }
}

//初始化输出GPIO
HW_GPIO_Status_e HW_GPIO_OUT_init(GPIO_index_e GPIO_index,GPIO_pull_e pull){
	/* 1. 检查索引是否越界 */
    if ((uint32_t)GPIO_index >= GPIO_MAP_COUNT)
    {
        return HW_GPIO_STATUS_INVALID_ARG;
    }

    /* 2. 从你的映射表取出实际端口和引脚 */
    const GPIO_GPIO_MAP_t *gpio_cfg = &GPIO_GPIO_MAP[GPIO_index];

    /* 3. 检查这个端口是否在当前芯片上存在 */
    if (gpio_cfg->gpio_port == NULL)
    {
        return HW_GPIO_STATUS_UNSUPPORTED;
    }

    /* 4. 打开对应端口的时钟 */
		HW_GPIO_EnableClock(gpio_cfg->gpio_port);

    /* 5. 用零初始化，避免 LL 结构体存在未赋值成员 */
    LL_GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = gpio_cfg->gpio_pin;
    gpio_init.Mode = LL_GPIO_MODE_OUTPUT;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		/*自定义上下拉*/
		switch(pull){
			case NO:gpio_init.Pull = LL_GPIO_PULL_NO;break;
			case UP:gpio_init.Pull = LL_GPIO_PULL_UP;break;
			case DOWN:gpio_init.Pull = LL_GPIO_PULL_DOWN;break;
			default:gpio_init.Pull = LL_GPIO_PULL_NO;break;
		}
    

    /* 6. 调用厂商 LL 函数；按实际返回类型判断 */
    if (LL_GPIO_Init(gpio_cfg->gpio_port, &gpio_init) != SUCCESS)
    {
        return HW_GPIO_STATUS_INIT_FAILED;
    }
		
		LOG_DEBUG("GPIO out init success\r\n");
    return HW_GPIO_STATUS_OK;
}

//初始化输入GPIO
HW_GPIO_Status_e HW_GPIO_INPUT_init(GPIO_index_e GPIO_index,GPIO_pull_e pull){
	
	/* 1. 检查索引是否越界 */
    if ((uint32_t)GPIO_index >= GPIO_MAP_COUNT)
    {
        return HW_GPIO_STATUS_INVALID_ARG;
    }

    /* 2. 从你的映射表取出实际端口和引脚 */
    const GPIO_GPIO_MAP_t *gpio_cfg = &GPIO_GPIO_MAP[GPIO_index];

    /* 3. 检查这个端口是否在当前芯片上存在 */
    if (gpio_cfg->gpio_port == NULL)
    {
        return HW_GPIO_STATUS_UNSUPPORTED;
    }

    /* 4. 打开对应端口的时钟 */
		HW_GPIO_EnableClock(gpio_cfg->gpio_port);

    /* 5. 用零初始化，避免 LL 结构体存在未赋值成员 */
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = gpio_cfg->gpio_pin;

		GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT; 
		/*自定义上下拉*/
		switch(pull){
			case NO:GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;break;
			case UP:GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;break;
			case DOWN:GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;break;
			default:GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;break;
		}

    /* 6. 调用厂商 LL 函数；按实际返回类型判断 */
    if (LL_GPIO_Init(gpio_cfg->gpio_port, &GPIO_InitStruct) != SUCCESS)
    {
        return HW_GPIO_STATUS_INIT_FAILED;
    }
		
    return HW_GPIO_STATUS_OK;
}

/*设置引脚电平状态*/
HW_GPIO_Status_e HW_GPIO_SET_Pin(GPIO_index_e GPIO_index,GPIO_mode_e mode){
    if ((uint32_t)GPIO_index >= GPIO_MAP_COUNT)
    {
        return HW_GPIO_STATUS_INVALID_ARG;
    }

    const GPIO_GPIO_MAP_t *gpio_cfg = &GPIO_GPIO_MAP[GPIO_index];

    if (gpio_cfg->gpio_port == NULL)
    {
        return HW_GPIO_STATUS_UNSUPPORTED;
    }

    switch (mode)
    {
        case LOW:
            LL_GPIO_ResetOutputPin(
                gpio_cfg->gpio_port,
                gpio_cfg->gpio_pin);
            break;

        case HIGH:
            LL_GPIO_SetOutputPin(
                gpio_cfg->gpio_port,
                gpio_cfg->gpio_pin);
            break;

        case TOGGLE:
            LL_GPIO_TogglePin(
                gpio_cfg->gpio_port,
                gpio_cfg->gpio_pin);
            break;

        default:
            return HW_GPIO_STATUS_INVALID_ARG;
    }

    return HW_GPIO_STATUS_OK;
}

/*单按键获取*/
HW_GPIO_Status_e HW_GPIO_Get_SingleKey(GPIO_index_e GPIO_index,uint8_t *out_level){
    if (out_level == NULL)
    {
        return HW_GPIO_STATUS_INVALID_ARG;
    }

    if ((uint32_t)GPIO_index >= GPIO_MAP_COUNT)
    {
        return HW_GPIO_STATUS_INVALID_ARG;
    }

    const GPIO_GPIO_MAP_t *gpio_cfg = &GPIO_GPIO_MAP[GPIO_index];

    if (gpio_cfg->gpio_port == NULL)
    {
        return HW_GPIO_STATUS_UNSUPPORTED;
    }

    *out_level = LL_GPIO_IsInputPinSet(
        gpio_cfg->gpio_port,
        gpio_cfg->gpio_pin) ? 1U : 0U;

    return HW_GPIO_STATUS_OK;
}

/* EXTI Line 0、1 共用的 NVIC 中断入口。 */
void EXTI0_1_IRQHandler(void)
{
    HW_GPIO_EXTI_IRQHandler(0U, 1U);
}

/* EXTI Line 2、3 共用的 NVIC 中断入口。 */
void EXTI2_3_IRQHandler(void)
{
    HW_GPIO_EXTI_IRQHandler(2U, 3U);
}

/* EXTI Line 4~8 使用同一个 NVIC 中断入口。 */
void EXTI4_15_IRQHandler(void)
{
    HW_GPIO_EXTI_IRQHandler(4U, 8U);
}

/* 默认 GPIO 中断回调；APP 层可用同名函数覆盖。 */
__weak void HW_GPIO_INPUT_IT_Callback(GPIO_index_e GPIO_index)
{
    (void)GPIO_index;
}

