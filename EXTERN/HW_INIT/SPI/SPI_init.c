#include "./HW_INIT/SPI/SPI_init.h"

/*使用例
 *HW_SPI_Status_e status;
 *status = HW_SPI_DMA_init(SPI_BUS1, SCK_A1, MOSI_A2, MISO_NULL);
 *
 *if (status == HW_SPI_STATUS_OK)
 *{
 *    uint8_t data[] = {0xAA, 0x55};
 *
 *    status = SPI_Transmit_DMA(data, sizeof(data));
 *    if (status == HW_SPI_STATUS_OK)
 *    {
 *        status = SPI_WaitAndCheckEndOfTransfer();
 *    }
 *}
 */

/*
 * SPI 参数统一配置区。
 *
 * CPOL / CPHA 与四种 SPI 模式的对应关系：
 * Mode 0：CPOL 低、CPHA 第一个边沿；
 * Mode 1：CPOL 低、CPHA 第二个边沿；
 * Mode 2：CPOL 高、CPHA 第一个边沿；
 * Mode 3：CPOL 高、CPHA 第二个边沿。
 * 当前默认值是 Mode 3。
 */
#ifndef HW_SPI_CPOL
#define HW_SPI_CPOL                     LL_SPI_POLARITY_HIGH
#endif
#ifndef HW_SPI_CPHA
#define HW_SPI_CPHA                     LL_SPI_PHASE_2EDGE
#endif

/*发送速率*/
#ifndef HW_SPI_BAUDRATE_PRESCALER
#define HW_SPI_BAUDRATE_PRESCALER       LL_SPI_BAUDRATEPRESCALER_DIV128
#endif

/*开关速率*/
#ifndef HW_SPI_GPIO_SPEED
#define HW_SPI_GPIO_SPEED               LL_GPIO_SPEED_FREQ_HIGH
#endif

/*等待时间*/
#ifndef HW_SPI_WAIT_COUNT
#define HW_SPI_WAIT_COUNT               1000000UL
#endif

/* 收发数据完成标志 */
volatile uint8_t txRxDataComplteFlag = RESET;

volatile uint8_t SPI_DMA_Flag = 0;//定义全局传输完成标志位

/* SCK、MOSI、MISO 共用的 GPIO 复用配置。 */
typedef struct
{
  GPIO_TypeDef *gpio_port;
  uint32_t gpio_pin;
  uint32_t gpio_alternate;
} SPI_PinConfig_t;

typedef struct
{
  SPI_index_e spi_index;
  SPI_SCK_Pin_e pin_index;
  SPI_TypeDef *spi_instance;
  SPI_PinConfig_t pin_config;
} SPI_SCK_MAP_t;

typedef struct
{
  SPI_index_e spi_index;
  SPI_MOSI_Pin_e pin_index;
  SPI_TypeDef *spi_instance;
  SPI_PinConfig_t pin_config;
} SPI_MOSI_MAP_t;

typedef struct
{
  SPI_index_e spi_index;
  SPI_MISO_Pin_e pin_index;
  SPI_TypeDef *spi_instance;
  SPI_PinConfig_t pin_config;
} SPI_MISO_MAP_t;

typedef struct
{
  const SPI_SCK_MAP_t *sck_map;
  uint32_t sck_map_count;
  const SPI_MOSI_MAP_t *mosi_map;
  uint32_t mosi_map_count;
  const SPI_MISO_MAP_t *miso_map;
  uint32_t miso_map_count;
} HW_SPI_ChipMap_t;

#define SPI_SCK_MAP(SPI_INDEX, PIN_INDEX, INSTANCE, PORT, PIN, AF) \
  {SPI_INDEX, PIN_INDEX, INSTANCE, {PORT, PIN, AF}}

#define SPI_MOSI_MAP(SPI_INDEX, PIN_INDEX, INSTANCE, PORT, PIN, AF) \
  {SPI_INDEX, PIN_INDEX, INSTANCE, {PORT, PIN, AF}}

#define SPI_MISO_MAP(SPI_INDEX, PIN_INDEX, INSTANCE, PORT, PIN, AF) \
  {SPI_INDEX, PIN_INDEX, INSTANCE, {PORT, PIN, AF}}

#if defined(PY32F030PRE) && defined(SPI1)

/* PY32F030 SPI1_SCK 复用引脚映射。 */
static const SPI_SCK_MAP_t SPI_SCK_MAP_TABLE[] =
{
#ifdef GPIOA
  SPI_SCK_MAP(SPI_BUS1, SCK_A1, SPI1, GPIOA, LL_GPIO_PIN_1, LL_GPIO_AF_0),
  SPI_SCK_MAP(SPI_BUS1, SCK_A2, SPI1, GPIOA, LL_GPIO_PIN_2, LL_GPIO_AF_10),
  SPI_SCK_MAP(SPI_BUS1, SCK_A5, SPI1, GPIOA, LL_GPIO_PIN_5, LL_GPIO_AF_0),
  SPI_SCK_MAP(SPI_BUS1, SCK_A9, SPI1, GPIOA, LL_GPIO_PIN_9, LL_GPIO_AF_10),
#endif
#ifdef GPIOB
  SPI_SCK_MAP(SPI_BUS1, SCK_B3, SPI1, GPIOB, LL_GPIO_PIN_3, LL_GPIO_AF_0),
#endif
};

/* PY32F030 SPI1_MOSI 复用引脚映射。 */
static const SPI_MOSI_MAP_t SPI_MOSI_MAP_TABLE[] =
{
#ifdef GPIOA
  SPI_MOSI_MAP(SPI_BUS1, MOSI_A1,  SPI1, GPIOA, LL_GPIO_PIN_1,  LL_GPIO_AF_10),
  SPI_MOSI_MAP(SPI_BUS1, MOSI_A2,  SPI1, GPIOA, LL_GPIO_PIN_2,  LL_GPIO_AF_0),
  SPI_MOSI_MAP(SPI_BUS1, MOSI_A3,  SPI1, GPIOA, LL_GPIO_PIN_3,  LL_GPIO_AF_10),
  SPI_MOSI_MAP(SPI_BUS1, MOSI_A7,  SPI1, GPIOA, LL_GPIO_PIN_7,  LL_GPIO_AF_0),
  SPI_MOSI_MAP(SPI_BUS1, MOSI_A8,  SPI1, GPIOA, LL_GPIO_PIN_8,  LL_GPIO_AF_10),
  SPI_MOSI_MAP(SPI_BUS1, MOSI_A12, SPI1, GPIOA, LL_GPIO_PIN_12, LL_GPIO_AF_0),
#endif
#ifdef GPIOB
  SPI_MOSI_MAP(SPI_BUS1, MOSI_B5, SPI1, GPIOB, LL_GPIO_PIN_5, LL_GPIO_AF_0),
#endif
};

/* PY32F030 SPI1_MISO 复用引脚映射。 */
static const SPI_MISO_MAP_t SPI_MISO_MAP_TABLE[] =
{
#ifdef GPIOA
  SPI_MISO_MAP(SPI_BUS1, MISO_A0,  SPI1, GPIOA, LL_GPIO_PIN_0,  LL_GPIO_AF_10),
  SPI_MISO_MAP(SPI_BUS1, MISO_A6,  SPI1, GPIOA, LL_GPIO_PIN_6,  LL_GPIO_AF_0),
  SPI_MISO_MAP(SPI_BUS1, MISO_A7,  SPI1, GPIOA, LL_GPIO_PIN_7,  LL_GPIO_AF_10),
  SPI_MISO_MAP(SPI_BUS1, MISO_A11, SPI1, GPIOA, LL_GPIO_PIN_11, LL_GPIO_AF_0),
  SPI_MISO_MAP(SPI_BUS1, MISO_A13, SPI1, GPIOA, LL_GPIO_PIN_13, LL_GPIO_AF_10),
#endif
#ifdef GPIOB
  SPI_MISO_MAP(SPI_BUS1, MISO_B4, SPI1, GPIOB, LL_GPIO_PIN_4, LL_GPIO_AF_0),
#endif
};

static const HW_SPI_ChipMap_t s_spi_chip_map =
{
  SPI_SCK_MAP_TABLE,
  sizeof(SPI_SCK_MAP_TABLE) / sizeof(SPI_SCK_MAP_TABLE[0]),
  SPI_MOSI_MAP_TABLE,
  sizeof(SPI_MOSI_MAP_TABLE) / sizeof(SPI_MOSI_MAP_TABLE[0]),
  SPI_MISO_MAP_TABLE,
  sizeof(SPI_MISO_MAP_TABLE) / sizeof(SPI_MISO_MAP_TABLE[0])
};

#else

/* 其他芯片型号的 SPI 引脚映射预留位置。 */
static const HW_SPI_ChipMap_t s_spi_chip_map =
{
  NULL, 0U,
  NULL, 0U,
  NULL, 0U
};

#endif

#undef SPI_SCK_MAP
#undef SPI_MOSI_MAP
#undef SPI_MISO_MAP

/* 当前完成初始化的 SPI 实例；NULL 表示尚未初始化。 */
static SPI_TypeDef *s_spi_active_instance[SPI_BUS_COUNT] = {NULL};

/* DMA 传输状态由中断写入，由 APP 侧等待函数读取。 */
static volatile HW_SPI_Status_e s_spi_transfer_status = HW_SPI_STATUS_NOT_READY;
static volatile uint8_t s_spi_transfer_busy = 0U;
static volatile uint8_t s_spi_rx_dma_active = 0U;
static uint8_t s_spi_full_duplex_available = 0U;
static uint8_t s_spi_tx_available = 0U;

/* 从芯片专属映射表中找出本次初始化所需的三根引脚。 */
static HW_SPI_Status_e HW_SPI_FindPinConfigs(SPI_index_e spi_index,
                                             SPI_SCK_Pin_e sck_pin,
                                             SPI_MOSI_Pin_e mosi_pin,
                                             SPI_MISO_Pin_e miso_pin,
                                             const SPI_PinConfig_t **sck_config,
                                             const SPI_PinConfig_t **mosi_config,
                                             const SPI_PinConfig_t **miso_config,
                                             SPI_TypeDef **spi_instance)
{
  uint32_t i;
  const SPI_SCK_MAP_t *sck_map = NULL;
  const SPI_MOSI_MAP_t *mosi_map = NULL;
  const SPI_MISO_MAP_t *miso_map = NULL;

  if ((sck_config == NULL) || (mosi_config == NULL) ||
      (miso_config == NULL) || (spi_instance == NULL))
  {
    return HW_SPI_STATUS_INVALID_ARG;
  }

  for (i = 0U; i < s_spi_chip_map.sck_map_count; i++)
  {
    if ((s_spi_chip_map.sck_map[i].spi_index == spi_index) &&
        (s_spi_chip_map.sck_map[i].pin_index == sck_pin))
    {
      sck_map = &s_spi_chip_map.sck_map[i];
      break;
    }
  }

  if (sck_map == NULL)
  {
    return HW_SPI_STATUS_UNSUPPORTED;
  }

  if (mosi_pin != MOSI_NULL)
  {
    for (i = 0U; i < s_spi_chip_map.mosi_map_count; i++)
    {
      if ((s_spi_chip_map.mosi_map[i].spi_index == spi_index) &&
          (s_spi_chip_map.mosi_map[i].pin_index == mosi_pin))
      {
        mosi_map = &s_spi_chip_map.mosi_map[i];
        break;
      }
    }

    if (mosi_map == NULL)
    {
      return HW_SPI_STATUS_UNSUPPORTED;
    }
  }

  if (miso_pin != MISO_NULL)
  {
    for (i = 0U; i < s_spi_chip_map.miso_map_count; i++)
    {
      if ((s_spi_chip_map.miso_map[i].spi_index == spi_index) &&
          (s_spi_chip_map.miso_map[i].pin_index == miso_pin))
      {
        miso_map = &s_spi_chip_map.miso_map[i];
        break;
      }
    }

    if (miso_map == NULL)
    {
      return HW_SPI_STATUS_UNSUPPORTED;
    }
  }

  if ((mosi_map == NULL) && (miso_map == NULL))
  {
    return HW_SPI_STATUS_INVALID_ARG;
  }

  if (((mosi_map != NULL) && (mosi_map->spi_instance != sck_map->spi_instance)) ||
      ((miso_map != NULL) && (miso_map->spi_instance != sck_map->spi_instance)))
  {
    return HW_SPI_STATUS_UNSUPPORTED;
  }

  /* 同一物理引脚不能同时承担两种 SPI 信号。 */
  if (((mosi_map != NULL) &&
       (mosi_map->pin_config.gpio_port == sck_map->pin_config.gpio_port) &&
       (mosi_map->pin_config.gpio_pin == sck_map->pin_config.gpio_pin)) ||
      ((miso_map != NULL) &&
       (miso_map->pin_config.gpio_port == sck_map->pin_config.gpio_port) &&
       (miso_map->pin_config.gpio_pin == sck_map->pin_config.gpio_pin)) ||
      ((mosi_map != NULL) && (miso_map != NULL) &&
       (mosi_map->pin_config.gpio_port == miso_map->pin_config.gpio_port) &&
       (mosi_map->pin_config.gpio_pin == miso_map->pin_config.gpio_pin)))
  {
    return HW_SPI_STATUS_INVALID_ARG;
  }

  *sck_config = &sck_map->pin_config;
  *mosi_config = (mosi_map == NULL) ? NULL : &mosi_map->pin_config;
  *miso_config = (miso_map == NULL) ? NULL : &miso_map->pin_config;
  *spi_instance = sck_map->spi_instance;

  return HW_SPI_STATUS_OK;
}

/* 按实际端口开 GPIO 时钟，避免把所有 GPIO 时钟都写死打开。 */
static HW_SPI_Status_e HW_SPI_EnableGPIOClock(GPIO_TypeDef *gpio_port)
{
  if (gpio_port == GPIOA)
  {
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  }
  else if (gpio_port == GPIOB)
  {
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
  }
  else
  {
    return HW_SPI_STATUS_UNSUPPORTED;
  }

  return HW_SPI_STATUS_OK;
}

/* 配置一根 SPI 复用 GPIO；上下拉固定为无上下拉。 */
static HW_SPI_Status_e HW_SPI_InitGPIOAlternate(const SPI_PinConfig_t *pin_config)
{
  LL_GPIO_InitTypeDef gpio_init = {0};
  HW_SPI_Status_e status;

  if (pin_config == NULL)
  {
    return HW_SPI_STATUS_INVALID_ARG;
  }

  status = HW_SPI_EnableGPIOClock(pin_config->gpio_port);
  if (status != HW_SPI_STATUS_OK)
  {
    return status;
  }

  gpio_init.Pin = pin_config->gpio_pin;
  gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
  gpio_init.Speed = HW_SPI_GPIO_SPEED;
  gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.Pull = LL_GPIO_PULL_NO;
  gpio_init.Alternate = pin_config->gpio_alternate;

  if (LL_GPIO_Init(pin_config->gpio_port, &gpio_init) != SUCCESS)
  {
    return HW_SPI_STATUS_GPIO_INIT_FAILED;
  }

  return HW_SPI_STATUS_OK;
}

/* 当前芯片系列的 SPI 外设时钟映射；后续型号在此处扩展。 */
static HW_SPI_Status_e HW_SPI_EnableClock(SPI_TypeDef *spi_instance)
{
  if (spi_instance == SPI1)
  {
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SPI1);
    return HW_SPI_STATUS_OK;
  }

  return HW_SPI_STATUS_UNSUPPORTED;
}

/* 等待 SPI 最后一个数据真正移出移位寄存器。 */
static HW_SPI_Status_e HW_SPI_WaitPeripheralIdle(void)
{
  uint32_t timeout = HW_SPI_WAIT_COUNT;

  while ((LL_SPI_GetTxFIFOLevel(SPI1) != LL_SPI_TX_FIFO_EMPTY) ||
         (LL_SPI_IsActiveFlag_BSY(SPI1) != 0U))
  {
    if (timeout == 0U)
    {
      return HW_SPI_STATUS_TIMEOUT;
    }

    timeout--;
  }

  return HW_SPI_STATUS_OK;
}

/* 结束或中止 DMA 传输，并向 APP 侧公布最终状态。 */
static void HW_SPI_FinishTransfer(HW_SPI_Status_e status)
{
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
  LL_SPI_DisableDMAReq_TX(SPI1);
  LL_SPI_DisableDMAReq_RX(SPI1);

  s_spi_transfer_status = status;
  s_spi_transfer_busy = 0U;
  s_spi_rx_dma_active = 0U;
  SPI_DMA_Flag = 0U;
  txRxDataComplteFlag = SET;
}

/**
  * @brief  SPI1配置函数
  * @param  头文件枚举参数
  * @retval SPI 初始化状态
  */
#if 0
/* 旧版固定引脚初始化实现：保留在此仅用于对照，不参与编译。 */
HW_SPI_Status_e HW_SPI_DMA_init_legacy(SPI_index_e spi_index)
{
	const SPI_GPIO_MAP_t *cfg;
	
  LL_SPI_InitTypeDef SPI_InitStruct = {0};		//创建SPI初始化结构体
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};	//创建GPIO初始化结构体
  LL_DMA_InitTypeDef DMA_InitStruct = {0};		//设定DMA结构体

  /* 检查 SPI 枚举和映射表，避免数组越界或使用未知外设。 */
  if ((uint32_t)spi_index >= (uint32_t)SPI_COUNT)
  {
    return HW_SPI_STATUS_INVALID_ARG;
  }

	cfg = &GPIO_MAP[spi_index];

  if ((cfg->spix != SPI1) ||
      (cfg->SCK_port == NULL) ||
      (cfg->SCK_pin == 0U))
  {
    return HW_SPI_STATUS_UNSUPPORTED;
  }

  /* 同一个 SPI 不允许在运行中重复初始化。 */
  if (s_spi_active_instance[spi_index] != NULL)
  {
    return HW_SPI_STATUS_ALREADY_INITIALIZED;
  }

  /* 使能时钟 */
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SPI1);		//使能SPI时钟
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);		//使能GPIOB时钟
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);		//使能GPIOB时钟
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);	//使能中断时钟
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);		//使能DMA时钟
  
  /**SPI1 引脚配置
  PA1   ------> SPI1_SCK
  当前映射：PA1 ------> SPI1_SCK，PA2 ------> SPI1_MOSI，MISO 未使用
  */
  GPIO_InitStruct.Pin = cfg->SCK_pin;                       //引脚号LL_GPIO_PIN_1
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;             //复用功能模式
  GPIO_InitStruct.Speed = HW_SPI_GPIO_SPEED;                  //超高速开关
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;      //推挽输出
  GPIO_InitStruct.Pull = HW_SPI_SCK_PULL;                    //默认拉高
  GPIO_InitStruct.Alternate = cfg->SCK_AF;                  //复用引脚
  if (LL_GPIO_Init(cfg->SCK_port, &GPIO_InitStruct) != SUCCESS)
  {
    return HW_SPI_STATUS_GPIO_INIT_FAILED;
  }

if (cfg->MISO_port != 0){
  GPIO_InitStruct.Pin = cfg->MISO_pin;                       //引脚号
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;             //复用功能模式
  GPIO_InitStruct.Speed = HW_SPI_GPIO_SPEED;                  //超高速开关
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;      //推挽输出
  GPIO_InitStruct.Pull = HW_SPI_MISO_PULL;                   //默认不上下拉
  GPIO_InitStruct.Alternate = cfg->MISO_AF;                  //复用引脚
  if (LL_GPIO_Init(cfg->MISO_port, &GPIO_InitStruct) != SUCCESS)
  {
    return HW_SPI_STATUS_GPIO_INIT_FAILED;
  }
}

if (cfg->MOSI_port != 0){
  GPIO_InitStruct.Pin = cfg->MOSI_pin;                       //引脚号
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;             //复用功能模式
  GPIO_InitStruct.Speed = HW_SPI_GPIO_SPEED;                  //超高速开关
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;      //推挽输出
  GPIO_InitStruct.Pull = HW_SPI_MOSI_PULL;                   //默认不上下拉
  GPIO_InitStruct.Alternate = cfg->MOSI_AF;                  //复用引脚
  if (LL_GPIO_Init(cfg->MOSI_port, &GPIO_InitStruct) != SUCCESS)
  {
    return HW_SPI_STATUS_GPIO_INIT_FAILED;
  }
}

  /* DMA配置 */
  /* 配置DMA请求映像 */
  LL_SYSCFG_SetDMARemap_CH1(LL_SYSCFG_DMA_MAP_SPI1_TX);			//将DMA通道1配置为TX
  LL_SYSCFG_SetDMARemap_CH2(LL_SYSCFG_DMA_MAP_SPI1_RX);			//将DMA通道2配置为RX
  
  /* DMA通道1初始化 */
  DMA_InitStruct.PeriphOrM2MSrcAddress  = 0x00000000U;													//标记外设向内部的地址
  DMA_InitStruct.MemoryOrM2MDstAddress  = 0x00000000U;                          //标记从内存到外设的地址
  DMA_InitStruct.Direction              = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;    //DMA朝向，内存到外设
  DMA_InitStruct.Mode                   = LL_DMA_MODE_NORMAL;                   //普通模式(传到指定个数自己停止)
  DMA_InitStruct.PeriphOrM2MSrcIncMode  = LL_DMA_PERIPH_NOINCREMENT;            //外设是否自增(不自增)
  DMA_InitStruct.MemoryOrM2MDstIncMode  = LL_DMA_MEMORY_INCREMENT;              //内存是否自增(自动增加)
  DMA_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;               //按8bit传输
  DMA_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;               //按8bit读取
  DMA_InitStruct.NbData                 = 0x00000000U;                          //搬运的个数，配合起始地址操作
  DMA_InitStruct.Priority               = LL_DMA_PRIORITY_VERYHIGH;             //DMA最高优先级
  if (LL_DMA_Init(DMA1, HW_SPI_DMA_TX_CHANNEL, &DMA_InitStruct) != SUCCESS)
  {
    return HW_SPI_STATUS_DMA_INIT_FAILED;
  }
  
  /* DMA通道2初始化 */
  DMA_InitStruct.PeriphOrM2MSrcAddress  = 0x00000000U;                          //标记外设向内部的地址
  DMA_InitStruct.MemoryOrM2MDstAddress  = 0x00000000U;                          //标记从内存到外设的地址
  DMA_InitStruct.Direction              = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;    //DMA朝向，外设到内存
  DMA_InitStruct.Mode                   = LL_DMA_MODE_NORMAL;                   //普通模式(传到指定个数自己停止)
  DMA_InitStruct.PeriphOrM2MSrcIncMode  = LL_DMA_PERIPH_NOINCREMENT;            //外设是否自增(不自增)
  DMA_InitStruct.MemoryOrM2MDstIncMode  = LL_DMA_MEMORY_INCREMENT;              //内存是否自增(自动增加)
  DMA_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;               //按8bit传输
  DMA_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;               //按8bit读取
  DMA_InitStruct.NbData                 = 0x00000000U;                          //搬运的个数，配合起始地址操作
  DMA_InitStruct.Priority               = LL_DMA_PRIORITY_LOW;                  //DMA最低优先级
  if (LL_DMA_Init(DMA1, HW_SPI_DMA_RX_CHANNEL, &DMA_InitStruct) != SUCCESS)
  {
    return HW_SPI_STATUS_DMA_INIT_FAILED;
  }
	
  /* 使能DMA的NVIC中断 */
  NVIC_SetPriority(DMA1_Channel1_IRQn, HW_SPI_DMA_IRQ_PRIORITY);			//设置发送中断优先级
  NVIC_EnableIRQ(DMA1_Channel1_IRQn);           //设置发送中断使能
  
  NVIC_SetPriority(DMA1_Channel2_3_IRQn, HW_SPI_DMA_IRQ_PRIORITY);		//设置接收中断优先级
  NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);					//设置接收中断使能

  /* SPI1 参数配置*/
	if(cfg->MISO_port == 0 || cfg->MOSI_port == 0)
		SPI_InitStruct.TransferDirection = LL_SPI_HALF_DUPLEX_TX;					//设定半双工模式
	else
		SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;				//设定全双工模式
  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;                       //设定主机模式
  SPI_InitStruct.DataWidth = HW_SPI_DATA_WIDTH;                    //设定数据宽度为1字节
  SPI_InitStruct.ClockPolarity = HW_SPI_CPOL;                      //设定时钟默认电平
  SPI_InitStruct.ClockPhase = HW_SPI_CPHA;                         //设定采样边沿
  SPI_InitStruct.NSS = HW_SPI_NSS_MODE;                            //设定软件NSS
  SPI_InitStruct.BaudRate = HW_SPI_BAUDRATE_PRESCALER;             //设定分频
  SPI_InitStruct.BitOrder = HW_SPI_BIT_ORDER;                      //设定高位优先发送
  if (LL_SPI_Init(SPI1, &SPI_InitStruct) != SUCCESS)
  {
    return HW_SPI_STATUS_INIT_FAILED;
  }
	
	s_spi_active_instance[spi_index] = SPI1;
  s_spi_transfer_status = HW_SPI_STATUS_NO_TRANSFER;
  s_spi_transfer_busy = 0U;
  s_spi_rx_dma_active = 0U;
  s_spi_full_duplex_available =
      ((cfg->MISO_port != NULL) && (cfg->MOSI_port != NULL)) ? 1U : 0U;
  SPI_DMA_Flag = 0U;
  txRxDataComplteFlag = RESET;

  return HW_SPI_STATUS_OK;
}
#endif

/**
  * @brief  按 SPI、SCK、MOSI、MISO 的逻辑枚举初始化 SPI1 与 DMA。
  * @param  mosi_pin / miso_pin 可传 MOSI_NULL / MISO_NULL，表示该方向不用。
  * @retval 初始化状态。
  */
HW_SPI_Status_e HW_SPI_DMA_init(SPI_index_e spi_index,
                                SPI_SCK_Pin_e sck_pin,
                                SPI_MOSI_Pin_e mosi_pin,
                                SPI_MISO_Pin_e miso_pin)
{
  const SPI_PinConfig_t *sck_config;
  const SPI_PinConfig_t *mosi_config;
  const SPI_PinConfig_t *miso_config;
  SPI_TypeDef *spi_instance;
  LL_SPI_InitTypeDef SPI_InitStruct = {0};
  LL_DMA_InitTypeDef DMA_InitStruct = {0};
  HW_SPI_Status_e status;

  if (((uint32_t)spi_index >= (uint32_t)SPI_BUS_COUNT) ||
      ((uint32_t)sck_pin >= (uint32_t)SCK_PIN_COUNT) ||
      ((uint32_t)mosi_pin >= (uint32_t)MOSI_PIN_COUNT) ||
      ((uint32_t)miso_pin >= (uint32_t)MISO_PIN_COUNT))
  {
    return HW_SPI_STATUS_INVALID_ARG;
  }

  if (s_spi_active_instance[spi_index] != NULL)
  {
    return HW_SPI_STATUS_ALREADY_INITIALIZED;
  }

  status = HW_SPI_FindPinConfigs(spi_index, sck_pin, mosi_pin, miso_pin,
                                  &sck_config, &mosi_config, &miso_config,
                                  &spi_instance);
  if (status != HW_SPI_STATUS_OK)
  {
    return status;
  }

  status = HW_SPI_EnableClock(spi_instance);
  if (status != HW_SPI_STATUS_OK)
  {
    return status;
  }

  status = HW_SPI_InitGPIOAlternate(sck_config);
  if (status != HW_SPI_STATUS_OK)
  {
    return status;
  }

  if (mosi_config != NULL)
  {
    status = HW_SPI_InitGPIOAlternate(mosi_config);
    if (status != HW_SPI_STATUS_OK)
    {
      return status;
    }
  }

  if (miso_config != NULL)
  {
    status = HW_SPI_InitGPIOAlternate(miso_config);
    if (status != HW_SPI_STATUS_OK)
    {
      return status;
    }
  }

  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

  /* 当前 PY32F030 SPI1 固定使用 CH1(TX) 与 CH2(RX)。 */
  LL_SYSCFG_SetDMARemap_CH1(LL_SYSCFG_DMA_MAP_SPI1_TX);
  LL_SYSCFG_SetDMARemap_CH2(LL_SYSCFG_DMA_MAP_SPI1_RX);

  DMA_InitStruct.PeriphOrM2MSrcAddress  = 0x00000000U;
  DMA_InitStruct.MemoryOrM2MDstAddress  = 0x00000000U;
  DMA_InitStruct.Direction              = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
  DMA_InitStruct.Mode                   = LL_DMA_MODE_NORMAL;
  DMA_InitStruct.PeriphOrM2MSrcIncMode  = LL_DMA_PERIPH_NOINCREMENT;
  DMA_InitStruct.MemoryOrM2MDstIncMode  = LL_DMA_MEMORY_INCREMENT;
  DMA_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
  DMA_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
  DMA_InitStruct.NbData                 = 0x00000000U;
  DMA_InitStruct.Priority               = LL_DMA_PRIORITY_VERYHIGH;
  if (LL_DMA_Init(DMA1, LL_DMA_CHANNEL_1, &DMA_InitStruct) != SUCCESS)
  {
    return HW_SPI_STATUS_DMA_INIT_FAILED;
  }

  DMA_InitStruct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
  DMA_InitStruct.Priority = LL_DMA_PRIORITY_LOW;
  if (LL_DMA_Init(DMA1, LL_DMA_CHANNEL_2, &DMA_InitStruct) != SUCCESS)
  {
    return HW_SPI_STATUS_DMA_INIT_FAILED;
  }

  /* 中断优先级固定为 1，和当前工程中 DMA 的优先级约定保持一致。 */
  NVIC_SetPriority(DMA1_Channel1_IRQn, 1U);
  NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  NVIC_SetPriority(DMA1_Channel2_3_IRQn, 1U);
  NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

  if (mosi_config == NULL)
  {
    SPI_InitStruct.TransferDirection = LL_SPI_HALF_DUPLEX_RX;
  }
  else if (miso_config == NULL)
  {
    SPI_InitStruct.TransferDirection = LL_SPI_HALF_DUPLEX_TX;
  }
  else
  {
    SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  }

  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT; /* 当前硬件层固定为 8 位数据。 */
  SPI_InitStruct.ClockPolarity = HW_SPI_CPOL;
  SPI_InitStruct.ClockPhase = HW_SPI_CPHA;
  SPI_InitStruct.NSS = LL_SPI_NSS_SOFT; /* 片选由应用层自行控制。 */
  SPI_InitStruct.BaudRate = HW_SPI_BAUDRATE_PRESCALER;
  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST; /* 当前硬件层固定为高位先行。 */
  if (LL_SPI_Init(spi_instance, &SPI_InitStruct) != SUCCESS)
  {
    return HW_SPI_STATUS_INIT_FAILED;
  }

  s_spi_active_instance[spi_index] = spi_instance;
  s_spi_transfer_status = HW_SPI_STATUS_NO_TRANSFER;
  s_spi_transfer_busy = 0U;
  s_spi_rx_dma_active = 0U;
  s_spi_full_duplex_available =
      ((mosi_config != NULL) && (miso_config != NULL)) ? 1U : 0U;
  s_spi_tx_available = (mosi_config != NULL) ? 1U : 0U;
  SPI_DMA_Flag = 0U;
  txRxDataComplteFlag = RESET;

  return HW_SPI_STATUS_OK;
}

/**
  * @brief  SPI1收发函数
  * @param  pTxData：发送数据缓冲区
  * @param  pRxData：接收数据缓冲区
  * @param  Size：收发数据的大小
  * @retval 启动 DMA 传输的状态；真正的传输结果由等待函数返回
  */
HW_SPI_Status_e SPI_TransmitReceive_DMA(uint8_t *pTxData, uint8_t *pRxData, uint16_t Size)
{
  uint32_t dataRegAddr;

  if ((pTxData == NULL) || (pRxData == NULL) || (Size == 0U))
  {
    return HW_SPI_STATUS_INVALID_ARG;
  }

  if (s_spi_active_instance[SPI_BUS1] == NULL)
  {
    return HW_SPI_STATUS_NOT_READY;
  }

  if (s_spi_full_duplex_available == 0U)
  {
    return HW_SPI_STATUS_UNSUPPORTED;
  }

  if (s_spi_transfer_busy != 0U)
  {
    return HW_SPI_STATUS_BUSY;
  }

  /* 收发数据未完成 */
  txRxDataComplteFlag = RESET;
  SPI_DMA_Flag = 1U;
  s_spi_transfer_status = HW_SPI_STATUS_BUSY;
  s_spi_transfer_busy = 1U;
  s_spi_rx_dma_active = 1U;
  
  /* 不使能SPI */
  LL_SPI_Disable(SPI1);
  
  /* 设置接收阈值为8bit */
  LL_SPI_SetRxFIFOThreshold(SPI1, LL_SPI_RX_FIFO_TH_QUARTER);
  
  /* 使能SPI的DMA接收中断 */
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
  
  LL_DMA_ClearFlag_GI2(DMA1);
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, Size);
  dataRegAddr = LL_SPI_DMA_GetRegAddr(SPI1);
  LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_2, dataRegAddr, (uint32_t)pRxData, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  
  LL_DMA_DisableIT_HT(DMA1, LL_DMA_CHANNEL_2);
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_2);
  LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_2);
  
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
  
  /* 使能SPI的DMA接收请求 */
  LL_SPI_EnableDMAReq_RX(SPI1);
  
  /* 使能SPI的DMA发送中断 */
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);						//失能DMA通道
  
  LL_DMA_ClearFlag_GI1(DMA1);																//清除标志位
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, Size);  //设置数据长度
  dataRegAddr = LL_SPI_DMA_GetRegAddr(SPI1);                //获取目标地址
  LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_1, (uint32_t)pTxData, dataRegAddr, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);//配置DMA，通道，数据起始，数据目标地址，数据朝向
  
  LL_DMA_DisableIT_HT(DMA1, LL_DMA_CHANNEL_1);  //失能中断半传输中断
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);   //使能传输完成中断
  LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_1);   //传输错误中断
											
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1); //使能传输通道
  
  /* 使能SPI */
  LL_SPI_Enable(SPI1);
  
  /* 使能SPI的DMA发送请求 */
  LL_SPI_EnableDMAReq_TX(SPI1);

  return HW_SPI_STATUS_OK;
}

/**
	* @brief  SPI1发送函数
  * @param  pTxData：发送数据缓冲区
  * @param  Size：收发数据的大小
  * @retval 启动 DMA 传输的状态；真正的传输结果由等待函数返回
  */
HW_SPI_Status_e SPI_Transmit_DMA(uint8_t *pTxData, uint16_t Size)
{
  uint32_t dataRegAddr;

  if ((pTxData == NULL) || (Size == 0U))
  {
    return HW_SPI_STATUS_INVALID_ARG;
  }

  if (s_spi_active_instance[SPI_BUS1] == NULL)
  {
    return HW_SPI_STATUS_NOT_READY;
  }

  /* 只有 MISO_NULL 的半双工发送模式可使用这个仅发送接口。 */
  if ((s_spi_tx_available == 0U) || (s_spi_full_duplex_available != 0U))
  {
    return HW_SPI_STATUS_UNSUPPORTED;
  }

  if (s_spi_transfer_busy != 0U)
  {
    return HW_SPI_STATUS_BUSY;
  }

  /* 收发数据未完成 */
  txRxDataComplteFlag = RESET;
	SPI_DMA_Flag = 1U;//正在传输
  s_spi_transfer_status = HW_SPI_STATUS_BUSY;
  s_spi_transfer_busy = 1U;
  s_spi_rx_dma_active = 0U;
  
  /* 不使能SPI */
  LL_SPI_Disable(SPI1);
  
  /* 仅发送时不启用 RX DMA 请求，避免接收 FIFO 无人读取。 */
  LL_SPI_DisableDMAReq_RX(SPI1);
  
  /* 使能SPI的DMA发送中断 */
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);						//失能DMA通道
  
  LL_DMA_ClearFlag_GI1(DMA1);																//清除标志位
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, Size);  //设置数据长度
  dataRegAddr = LL_SPI_DMA_GetRegAddr(SPI1);                         //获取目标地址
  LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_1, (uint32_t)pTxData, dataRegAddr, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);//配置DMA，通道，数据起始，数据目标地址，数据朝向
  
  LL_DMA_DisableIT_HT(DMA1, LL_DMA_CHANNEL_1);  //失能中断半传输中断
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);   //使能传输完成中断
  LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_1);   //传输错误中断
											
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1); //使能传输通道
  
  /* 使能SPI */
  LL_SPI_Enable(SPI1);
  
  /* 使能SPI的DMA发送请求 */
  LL_SPI_EnableDMAReq_TX(SPI1);

  return HW_SPI_STATUS_OK;
}


/**
  * @brief  SPI的DMA发送回调函数
  * @param  无
  * @retval 无
  */
__weak void SPI_DmaTxIRQCallback(void)
{
	
}

/**
  * @brief  SPI的DMA接收回调函数
  * @param  无
  * @retval 无
  */
void SPI_DmaSpiRxIRQCallback(void)
{
  if ((LL_DMA_IsActiveFlag_TE2(DMA1) == 1U) &&
      (LL_DMA_IsEnabledIT_TE(DMA1, LL_DMA_CHANNEL_2) == 1U))
  {
    LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_2);
    LL_DMA_DisableIT_TE(DMA1, LL_DMA_CHANNEL_2);
    LL_DMA_ClearFlag_TE2(DMA1);
    HW_SPI_FinishTransfer(HW_SPI_STATUS_DMA_ERROR);
    return;
  }

  if ((LL_DMA_IsActiveFlag_TC2(DMA1) == 1U) &&
      (LL_DMA_IsEnabledIT_TC(DMA1, LL_DMA_CHANNEL_2) == 1U))
  {
    /* 关传输完成中断 */
    LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_2);
    LL_DMA_DisableIT_TE(DMA1, LL_DMA_CHANNEL_2);
    
    /* 清传输完成标志位 */
    LL_DMA_ClearFlag_TC2(DMA1);
    
    /* DMA 完成后不在中断内死等，最后一个字节由 APP 等待函数确认。 */
    HW_SPI_FinishTransfer(HW_SPI_STATUS_OK);
  }
}

/**
  * @brief  SPI1等待传输完成，并校验数据
  * @param  无
  * @retval SPI DMA 传输的最终状态
  */
HW_SPI_Status_e SPI_WaitAndCheckEndOfTransfer(void)
{
  uint32_t timeout = HW_SPI_WAIT_COUNT;
  HW_SPI_Status_e status;

  if (s_spi_active_instance[SPI_BUS1] == NULL)
  {
    return HW_SPI_STATUS_NOT_READY;
  }

  if (s_spi_transfer_status == HW_SPI_STATUS_NO_TRANSFER)
  {
    return HW_SPI_STATUS_NO_TRANSFER;
  }

  /* 1 - 等待传输结束 */
  while (txRxDataComplteFlag != SET)
  {
    if (s_spi_transfer_status != HW_SPI_STATUS_BUSY)
    {
      return s_spi_transfer_status;
    }

    if (timeout == 0U)
    {
      HW_SPI_FinishTransfer(HW_SPI_STATUS_TIMEOUT);
      return HW_SPI_STATUS_TIMEOUT;
    }

    timeout--;
  }

  status = s_spi_transfer_status;
  if (status != HW_SPI_STATUS_OK)
  {
    return status;
  }

  /* 对仅发送 DMA，再确认最后一个字节已经从 SPI 移出。 */
  status = HW_SPI_WaitPeripheralIdle();
  if (status != HW_SPI_STATUS_OK)
  {
    HW_SPI_FinishTransfer(status);
    return status;
  }

  /* 2 - 比较发送数据和接收数据 */
//  if(APP_Buffercmp8((uint8_t*)aTxBuffer, (uint8_t*)aRxBuffer, ubNbDataToTransmit))
//  {
//    /* 错误处理 */
//    APP_LedBlinking();
//  }
//  else
//  {
//    /* 如果数据接收到，则打开 LED */
//    //BSP_LED_On(LED_GREEN);
//  }

  return HW_SPI_STATUS_OK;
}

/**
  * @brief  错误执行函数
  * @param  无
  * @retval 无
  */
void SPI_ErrorHandler(void)
{
  /* 无限循环 */
  while (1)
  {
  }
}

//DMA中断跳转
/**
  * @brief This function handles DMA channel 1 interrupt.
  */
void DMA1_Channel1_IRQHandler(void)
{
	if ((LL_DMA_IsActiveFlag_TE1(DMA1) == 1U) &&
      (LL_DMA_IsEnabledIT_TE(DMA1, LL_DMA_CHANNEL_1) == 1U))
  {
    LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_DisableIT_TE(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_ClearFlag_TE1(DMA1);
    HW_SPI_FinishTransfer(HW_SPI_STATUS_DMA_ERROR);
    return;
  }

	/*检查传输标志位，并清除*/
  if ((LL_DMA_IsActiveFlag_TC1(DMA1) == 1) && (LL_DMA_IsEnabledIT_TC(DMA1, LL_DMA_CHANNEL_1) == 1))//传输完成标志，传输完成中断是否打开
  {
    /* 关传输完成中断 */
    LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_DisableIT_TE(DMA1, LL_DMA_CHANNEL_1);
    
    /* 清传输完成标志位 */
    LL_DMA_ClearFlag_TC1(DMA1);

    /* 全双工时必须等 RX DMA 完成，不能由 TX DMA 提前结束整笔传输。 */
    if (s_spi_rx_dma_active == 0U)
    {
      HW_SPI_FinishTransfer(HW_SPI_STATUS_OK);
    }

    SPI_DmaTxIRQCallback();//callback函数
  }
}

/**
  * @brief This function handles DMA channel 2、3 interrupt.
  */
void DMA1_Channel2_3_IRQHandler(void)
{
  SPI_DmaSpiRxIRQCallback();
}



