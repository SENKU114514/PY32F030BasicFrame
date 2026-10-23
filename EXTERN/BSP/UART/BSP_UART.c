#include "BSP/UART/BSP_UART.h"

/* =============================== 内部配置 =============================== */

#define BSP_UART_TIM			TIM14							//位延时使用 TIM14
#define BSP_UART_TIM_CLK		LL_APB1_GRP2_PERIPH_TIM14		//TIM14 时钟
#define BSP_UART_CYCLES_MAX		40000U							//单个位的最大计数值

#ifndef BSP_UART_SOFT_LOOP_CYCLES
#define BSP_UART_SOFT_LOOP_CYCLES	6U							//软件循环一次约几个 CPU 周期
#endif

/* =============================== 内部状态 =============================== */

static GPIO_TypeDef *s_tx_port;		//发送端口
static uint32_t s_tx_mask;			//发送引脚掩码
static GPIO_TypeDef *s_rx_port;		//接收端口
static uint32_t s_rx_mask;			//接收引脚掩码
static uint32_t s_baud;				//波特率
static uint16_t s_bit_cycles;		//一个位对应的 CPU 周期数
static uint8_t s_tim_ready;			//TIM14 是否已启动

/* =============================== 内部函数 =============================== */

/* 由引脚编号取得端口和掩码 */
static uint8_t BSP_UART_GetPort(GPIO_index_e pin, GPIO_TypeDef **port, uint32_t *mask)
{
	uint8_t index = (uint8_t)pin;

	if (index < 16U)
	{
		*port = GPIOA;
	}
#if defined(GPIOB)
	else if (index < 32U)
	{
		*port = GPIOB;
	}
#endif
#if defined(GPIOC)
	else if (index < 48U)
	{
		*port = GPIOC;
	}
#endif
#if defined(GPIOF)
	else if (index < 64U)
	{
		*port = GPIOF;
	}
#endif
	else
	{
		return 0U;
	}

	*mask = (uint32_t)1U << (index & 0x0FU);
	return 1U;
}

/* 启动 TIM14，作为 1 个计数 = 1 个 CPU 周期的时基 */
static void BSP_UART_StartTimer(void)
{
	if (s_tim_ready != 0U)
	{
		return;
	}

	LL_APB1_GRP2_EnableClock(BSP_UART_TIM_CLK);

	LL_TIM_SetPrescaler(BSP_UART_TIM, 0U);
	LL_TIM_SetCounterMode(BSP_UART_TIM, LL_TIM_COUNTERMODE_UP);
	LL_TIM_SetAutoReload(BSP_UART_TIM, 0xFFFFU);
	LL_TIM_GenerateEvent_UPDATE(BSP_UART_TIM);
	LL_TIM_EnableCounter(BSP_UART_TIM);

	s_tim_ready = 1U;
}

/* 定时器延时：按 CPU 周期计数 */
static void BSP_UART_DelayTim(uint16_t cycles)
{
	uint16_t start = (uint16_t)LL_TIM_GetCounter(BSP_UART_TIM);

	while ((uint16_t)((uint16_t)LL_TIM_GetCounter(BSP_UART_TIM) - start) < cycles)
	{
	}
}

/* 软件延时：按 CPU 周期折算成循环次数 */
static void BSP_UART_DelaySoft(uint16_t cycles)
{
	volatile uint16_t count = (uint16_t)(cycles / BSP_UART_SOFT_LOOP_CYCLES);

	while (count != 0U)
	{
		__NOP();
		count--;
	}
}

/* 发送一个字节：起始位 + 8 数据位 + 停止位 */
static void BSP_UART_TxByte(uint8_t byte, uint8_t use_tim)
{
	uint32_t primask = __get_PRIMASK();
	uint8_t i;

	__disable_irq();											//保证位时序

	LL_GPIO_ResetOutputPin(s_tx_port, s_tx_mask);				//起始位

	if (use_tim != 0U)
	{
		BSP_UART_DelayTim(s_bit_cycles);
	}
	else
	{
		BSP_UART_DelaySoft(s_bit_cycles);
	}

	for (i = 0U; i < 8U; i++)
	{
		if ((byte & 0x01U) != 0U)
		{
			LL_GPIO_SetOutputPin(s_tx_port, s_tx_mask);
		}
		else
		{
			LL_GPIO_ResetOutputPin(s_tx_port, s_tx_mask);
		}

		if (use_tim != 0U)
		{
			BSP_UART_DelayTim(s_bit_cycles);
		}
		else
		{
			BSP_UART_DelaySoft(s_bit_cycles);
		}

		byte >>= 1;
	}

	LL_GPIO_SetOutputPin(s_tx_port, s_tx_mask);					//停止位

	if (use_tim != 0U)
	{
		BSP_UART_DelayTim(s_bit_cycles);
	}
	else
	{
		BSP_UART_DelaySoft(s_bit_cycles);
	}

	if (primask == 0U)
	{
		__enable_irq();
	}
}

/* 接收一个字节：轮询起始位，再在各数据位中心采样 */
static uint8_t BSP_UART_RxByte(uint32_t timeout_ms, uint8_t *out)
{
	uint32_t start = get_mag_tick();
	uint16_t half = (uint16_t)(s_bit_cycles / 2U);
	uint8_t data = 0U;
	uint8_t i;

	/* 等待起始位（低电平） */
	while (LL_GPIO_IsInputPinSet(s_rx_port, s_rx_mask) != 0U)
	{
		if ((uint32_t)(get_mag_tick() - start) >= timeout_ms)
		{
			return 0U;
		}
	}

	BSP_UART_DelayTim((uint16_t)(s_bit_cycles + half));			//对齐到最低位中心

	for (i = 0U; i < 8U; i++)
	{
		data >>= 1;

		if (LL_GPIO_IsInputPinSet(s_rx_port, s_rx_mask) != 0U)
		{
			data |= 0x80U;
		}

		BSP_UART_DelayTim(s_bit_cycles);
	}

	*out = data;
	return 1U;
}

/* =============================== 对外接口 =============================== */

/* 初始化：指定发送引脚、接收引脚和波特率 */
BSP_UART_State_e BSP_UART_INIT(GPIO_index_e tx_pin, GPIO_index_e rx_pin, uint32_t baud)
{
	GPIO_TypeDef *port;
	uint32_t mask;
	uint16_t cycles;

	if ((baud == 0U) || (tx_pin == rx_pin))
	{
		return BSP_UART_FAIL;
	}

	cycles = (uint16_t)(SystemCoreClock / baud);

	if ((cycles == 0U) || (cycles > BSP_UART_CYCLES_MAX))
	{
		return BSP_UART_FAIL;
	}

	if (HW_GPIO_OUT_init(tx_pin, NO) != HW_GPIO_STATUS_OK)
	{
		return BSP_UART_FAIL;
	}

	if (HW_GPIO_INPUT_init(rx_pin, UP) != HW_GPIO_STATUS_OK)
	{
		return BSP_UART_FAIL;
	}

	if (BSP_UART_GetPort(tx_pin, &port, &mask) == 0U)
	{
		return BSP_UART_FAIL;
	}

	s_tx_port = port;
	s_tx_mask = mask;
	LL_GPIO_SetOutputPin(s_tx_port, s_tx_mask);				//空闲电平为高

	if (BSP_UART_GetPort(rx_pin, &port, &mask) == 0U)
	{
		return BSP_UART_FAIL;
	}

	s_rx_port = port;
	s_rx_mask = mask;

	s_baud = baud;
	s_bit_cycles = cycles;

	return BSP_UART_OK;
}

/* 发送：使用定时器精确定时 */
BSP_UART_State_e BSP_UART_TIM_SEND(const uint8_t *data, uint16_t length)
{
	uint16_t i;

	if ((data == NULL) || (length == 0U) || (s_baud == 0U))
	{
		return BSP_UART_FAIL;
	}

	BSP_UART_StartTimer();

	for (i = 0U; i < length; i++)
	{
		BSP_UART_TxByte(data[i], 1U);
	}

	return BSP_UART_OK;
}

/* 发送：使用软件循环定时，不占用定时器 */
BSP_UART_State_e BSP_UART_SOFT_SEND(const uint8_t *data, uint16_t length)
{
	uint16_t i;

	if ((data == NULL) || (length == 0U) || (s_baud == 0U))
	{
		return BSP_UART_FAIL;
	}

	for (i = 0U; i < length; i++)
	{
		BSP_UART_TxByte(data[i], 0U);
	}

	return BSP_UART_OK;
}

/* 接收：使用定时器采样，阻塞到收满或超时 */
uint16_t BSP_UART_RECEIVE(uint8_t *data, uint16_t capacity, uint32_t timeout_ms)
{
	uint32_t start;
	uint32_t elapsed;
	uint16_t count = 0U;

	if ((data == NULL) || (capacity == 0U) || (s_baud == 0U))
	{
		return 0U;
	}

	BSP_UART_StartTimer();

	start = get_mag_tick();

	while (count < capacity)
	{
		elapsed = (uint32_t)(get_mag_tick() - start);

		if (elapsed >= timeout_ms)
		{
			break;
		}

		if (BSP_UART_RxByte(timeout_ms - elapsed, &data[count]) == 0U)
		{
			break;
		}

		count++;
	}

	return count;
}
