#include "./HW_INIT/I2C/I2C_init.h"


/*
 * I2C 硬件层功能：
 *
 * 1. 根据枚举索引自动查找 SCL、SDA 的 GPIO 和复用功能；
 * 2. 检查两个引脚是否能够连接到同一个 I2C 外设；
 * 3. 初始化 I2C 主机和开漏 GPIO；
 * 4. 提供带错误判断和有限等待的轮询写、读、写后读接口。
 *
 * 当前版本只实现 PY32F030 的 I2C1 主机轮询模式。
 * PY32F002B 和其他芯片已经保留独立映射入口，后续可以扩展。
 */


/* 每次等待硬件标志的最大循环次数，避免硬件异常时程序永久卡死。 */
#define HW_I2C_WAIT_COUNT 1000000UL

/* 最后三字节接收时全局中断处于关闭状态，因此使用更短的专用等待上限。 */
#define HW_I2C_CRITICAL_WAIT_COUNT 10000UL

/* 当前首版支持的两种 I2C 总线速度。 */
#define HW_I2C_STANDARD_CLOCK_SPEED 100000UL
#define HW_I2C_FAST_CLOCK_SPEED     400000UL

/* 标准模式和快速模式所需的最小 I2C 外设时钟。 */
#define HW_I2C_STANDARD_MODE_MIN_PCLK  2000000UL
#define HW_I2C_FAST_MODE_MIN_PCLK      9000000UL


/* I2C SCL 或 SDA 引脚的通用 GPIO 配置。 */
typedef struct
{
    GPIO_TypeDef *gpio_port;
    uint32_t gpio_pin;
    uint32_t gpio_alternate;
} I2C_PinConfig_t;


/* SCL 引脚索引与实际硬件资源之间的映射。 */
typedef struct
{
    I2C_index_e i2c_index;
    I2C_SCL_Pin_e pin_index;
    I2C_TypeDef *i2c_instance;
    I2C_PinConfig_t pin_config;
} I2C_SCL_MAP_t;


/* SDA 引脚索引与实际硬件资源之间的映射。 */
typedef struct
{
    I2C_index_e i2c_index;
    I2C_SDA_Pin_e pin_index;
    I2C_TypeDef *i2c_instance;
    I2C_PinConfig_t pin_config;
} I2C_SDA_MAP_t;


/* 当前芯片使用的 SCL、SDA 映射表集合。 */
typedef struct
{
    const I2C_SCL_MAP_t *scl_map;
    uint32_t scl_map_count;

    const I2C_SDA_MAP_t *sda_map;
    uint32_t sda_map_count;
} HW_I2C_ChipMap_t;


/* 映射表初始化宏，只在本文件中使用。 */
#define I2C_SCL_MAP(I2C_INDEX, PIN_INDEX, INSTANCE, PORT, PIN, AF) \
    {I2C_INDEX, PIN_INDEX, INSTANCE, {PORT, PIN, AF}}

#define I2C_SDA_MAP(I2C_INDEX, PIN_INDEX, INSTANCE, PORT, PIN, AF) \
    {I2C_INDEX, PIN_INDEX, INSTANCE, {PORT, PIN, AF}}


#if defined(PY32F030PRE) && defined(I2C1)


/* PY32F030 的 I2C1_SCL 引脚复用映射表。 */
static const I2C_SCL_MAP_t I2C_SCL_MAP_TABLE[] =
{
#ifdef GPIOA
    I2C_SCL_MAP(I2C_BUS1, SCL_A3,  I2C1, GPIOA, LL_GPIO_PIN_3,  LL_GPIO_AF12_I2C),
    I2C_SCL_MAP(I2C_BUS1, SCL_A8,  I2C1, GPIOA, LL_GPIO_PIN_8,  LL_GPIO_AF12_I2C),
    I2C_SCL_MAP(I2C_BUS1, SCL_A9,  I2C1, GPIOA, LL_GPIO_PIN_9,  LL_GPIO_AF6_I2C),
    I2C_SCL_MAP(I2C_BUS1, SCL_A10, I2C1, GPIOA, LL_GPIO_PIN_10, LL_GPIO_AF12_I2C),
    I2C_SCL_MAP(I2C_BUS1, SCL_A11, I2C1, GPIOA, LL_GPIO_PIN_11, LL_GPIO_AF6_I2C),
#endif

#ifdef GPIOB
    I2C_SCL_MAP(I2C_BUS1, SCL_B6,  I2C1, GPIOB, LL_GPIO_PIN_6,  LL_GPIO_AF6_I2C),
    I2C_SCL_MAP(I2C_BUS1, SCL_B8,  I2C1, GPIOB, LL_GPIO_PIN_8,  LL_GPIO_AF6_I2C),
#endif

#ifdef GPIOF
    I2C_SCL_MAP(I2C_BUS1, SCL_F1,  I2C1, GPIOF, LL_GPIO_PIN_1,  LL_GPIO_AF12_I2C),
#endif
};


/* PY32F030 的 I2C1_SDA 引脚复用映射表。 */
static const I2C_SDA_MAP_t I2C_SDA_MAP_TABLE[] =
{
#ifdef GPIOA
    I2C_SDA_MAP(I2C_BUS1, SDA_A2,  I2C1, GPIOA, LL_GPIO_PIN_2,  LL_GPIO_AF12_I2C),
    I2C_SDA_MAP(I2C_BUS1, SDA_A7,  I2C1, GPIOA, LL_GPIO_PIN_7,  LL_GPIO_AF12_I2C),
    I2C_SDA_MAP(I2C_BUS1, SDA_A9,  I2C1, GPIOA, LL_GPIO_PIN_9,  LL_GPIO_AF12_I2C),
    I2C_SDA_MAP(I2C_BUS1, SDA_A10, I2C1, GPIOA, LL_GPIO_PIN_10, LL_GPIO_AF6_I2C),
    I2C_SDA_MAP(I2C_BUS1, SDA_A12, I2C1, GPIOA, LL_GPIO_PIN_12, LL_GPIO_AF6_I2C),
#endif

#ifdef GPIOB
    I2C_SDA_MAP(I2C_BUS1, SDA_B7,  I2C1, GPIOB, LL_GPIO_PIN_7,  LL_GPIO_AF6_I2C),
    I2C_SDA_MAP(I2C_BUS1, SDA_B8,  I2C1, GPIOB, LL_GPIO_PIN_8,  LL_GPIO_AF12_I2C),
#endif

#ifdef GPIOF
    I2C_SDA_MAP(I2C_BUS1, SDA_F0,  I2C1, GPIOF, LL_GPIO_PIN_0,  LL_GPIO_AF12_I2C),
#endif
};


static const HW_I2C_ChipMap_t s_i2c_chip_map =
{
    I2C_SCL_MAP_TABLE,
    sizeof(I2C_SCL_MAP_TABLE) / sizeof(I2C_SCL_MAP_TABLE[0]),

    I2C_SDA_MAP_TABLE,
    sizeof(I2C_SDA_MAP_TABLE) / sizeof(I2C_SDA_MAP_TABLE[0])
};


#elif defined(PY32F002BPRE)


/*
 * PY32F002B 扩展位置。
 * 当前没有加入未经该芯片手册确认的 I2C 引脚映射。
 */
static const HW_I2C_ChipMap_t s_i2c_chip_map =
{
    NULL,
    0U,
    NULL,
    0U
};


#else


/* 当前芯片型号暂未添加经过确认的 I2C 引脚映射。 */
static const HW_I2C_ChipMap_t s_i2c_chip_map =
{
    NULL,
    0U,
    NULL,
    0U
};


#endif


/* 映射宏使用完毕后立即取消，避免影响其他文件。 */
#undef I2C_SCL_MAP
#undef I2C_SDA_MAP


/* 保存每个逻辑 I2C 当前绑定的实际外设；NULL 表示尚未初始化。 */
static I2C_TypeDef *s_i2c_active_instance[I2C_BUS_COUNT] =
{
    NULL
};


/* 保存首次成功初始化时使用的引脚和速度，用于阻止危险的运行中换脚。 */
static I2C_SCL_Pin_e s_i2c_active_scl_pin[I2C_BUS_COUNT];
static I2C_SDA_Pin_e s_i2c_active_sda_pin[I2C_BUS_COUNT];
static uint32_t s_i2c_active_clock_speed[I2C_BUS_COUNT];


/* 防止主循环和中断等不同调用者同时操作同一个 I2C 外设。 */
static volatile uint8_t s_i2c_transfer_busy[I2C_BUS_COUNT] =
{
    0U
};


/* 等待的 I2C 硬件标志类型。 */
typedef enum
{
    HW_I2C_WAIT_SB = 0,
    HW_I2C_WAIT_ADDR,
    HW_I2C_WAIT_TXE,
    HW_I2C_WAIT_RXNE,
    HW_I2C_WAIT_BTF,
} HW_I2C_WaitFlag_e;


/* 根据 I2C 编号和 SCL、SDA 索引查找实际硬件配置。 */
static HW_I2C_Status_e HW_I2C_FindPinConfigs(
    I2C_index_e I2C_index,
    I2C_SCL_Pin_e SCL_Pin,
    I2C_SDA_Pin_e SDA_Pin,
    const I2C_PinConfig_t **scl_config,
    const I2C_PinConfig_t **sda_config,
    I2C_TypeDef **i2c_instance)
{
    uint32_t index;
    const I2C_SCL_MAP_t *scl_map = NULL;
    const I2C_SDA_MAP_t *sda_map = NULL;

    if ((scl_config == NULL) ||
        (sda_config == NULL) ||
        (i2c_instance == NULL))
    {
        return HW_I2C_STATUS_INVALID_ARG;
    }

    for (index = 0U; index < s_i2c_chip_map.scl_map_count; index++)
    {
        if ((s_i2c_chip_map.scl_map[index].i2c_index == I2C_index) &&
            (s_i2c_chip_map.scl_map[index].pin_index == SCL_Pin))
        {
            scl_map = &s_i2c_chip_map.scl_map[index];
            break;
        }
    }

    for (index = 0U; index < s_i2c_chip_map.sda_map_count; index++)
    {
        if ((s_i2c_chip_map.sda_map[index].i2c_index == I2C_index) &&
            (s_i2c_chip_map.sda_map[index].pin_index == SDA_Pin))
        {
            sda_map = &s_i2c_chip_map.sda_map[index];
            break;
        }
    }

    if ((scl_map == NULL) ||
        (sda_map == NULL) ||
        (scl_map->i2c_instance != sda_map->i2c_instance))
    {
        return HW_I2C_STATUS_UNSUPPORTED;
    }

    /* 同一个物理引脚不能同时作为 SCL 和 SDA。 */
    if ((scl_map->pin_config.gpio_port == sda_map->pin_config.gpio_port) &&
        (scl_map->pin_config.gpio_pin == sda_map->pin_config.gpio_pin))
    {
        return HW_I2C_STATUS_UNSUPPORTED;
    }

    *scl_config = &scl_map->pin_config;
    *sda_config = &sda_map->pin_config;
    *i2c_instance = scl_map->i2c_instance;

    return HW_I2C_STATUS_OK;
}


/* 使能实际 I2C 外设对应的 APB 时钟。 */
static HW_I2C_Status_e HW_I2C_EnableClock(I2C_TypeDef *i2c_instance)
{
#ifdef I2C1
    if (i2c_instance == I2C1)
    {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
        return HW_I2C_STATUS_OK;
    }
#endif

    return HW_I2C_STATUS_UNSUPPORTED;
}


/* 使能指定 GPIO 端口的时钟。 */
static HW_I2C_Status_e HW_I2C_EnableGPIOClock(GPIO_TypeDef *gpio_port)
{
    if (gpio_port == NULL)
    {
        return HW_I2C_STATUS_INVALID_ARG;
    }

#ifdef GPIOA
    if (gpio_port == GPIOA)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
        return HW_I2C_STATUS_OK;
    }
#endif

#ifdef GPIOB
    if (gpio_port == GPIOB)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
        return HW_I2C_STATUS_OK;
    }
#endif

#ifdef GPIOF
    if (gpio_port == GPIOF)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);
        return HW_I2C_STATUS_OK;
    }
#endif

    return HW_I2C_STATUS_UNSUPPORTED;
}


/* 把一个 SCL 或 SDA 引脚配置为复用开漏输出。 */
static HW_I2C_Status_e HW_I2C_InitGPIOAlternate(
    const I2C_PinConfig_t *pin_config)
{
    LL_GPIO_InitTypeDef gpio_init = {0};
    HW_I2C_Status_e status;

    if (pin_config == NULL)
    {
        return HW_I2C_STATUS_INVALID_ARG;
    }

    status = HW_I2C_EnableGPIOClock(pin_config->gpio_port);
    if (status != HW_I2C_STATUS_OK)
    {
        return status;
    }

    gpio_init.Pin = pin_config->gpio_pin;
    gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    gpio_init.Pull = LL_GPIO_PULL_UP;
    gpio_init.Alternate = pin_config->gpio_alternate;

    if (LL_GPIO_Init(pin_config->gpio_port, &gpio_init) != SUCCESS)
    {
        return HW_I2C_STATUS_GPIO_INIT_FAILED;
    }

    return HW_I2C_STATUS_OK;
}


/* 原子地占用一个 I2C，避免两个调用过程交叉修改硬件状态。 */
static HW_I2C_Status_e HW_I2C_Lock(I2C_index_e I2C_index)
{
    uint32_t interrupt_state;

    interrupt_state = __get_PRIMASK();
    __disable_irq();

    if (s_i2c_transfer_busy[I2C_index] != 0U)
    {
        if ((interrupt_state & 1U) == 0U)
        {
            __enable_irq();
        }

        return HW_I2C_STATUS_BUSY;
    }

    s_i2c_transfer_busy[I2C_index] = 1U;

    if ((interrupt_state & 1U) == 0U)
    {
        __enable_irq();
    }

    return HW_I2C_STATUS_OK;
}


/* 释放由 HW_I2C_Lock 占用的 I2C。 */
static void HW_I2C_Unlock(I2C_index_e I2C_index)
{
    uint32_t interrupt_state;

    interrupt_state = __get_PRIMASK();
    __disable_irq();

    s_i2c_transfer_busy[I2C_index] = 0U;

    if ((interrupt_state & 1U) == 0U)
    {
        __enable_irq();
    }
}


/*
 * 检查 I2C 的通信错误标志。
 *
 * 这里只识别错误，不立即清除：
 * AF、BERR、OVR 发生在本机主机事务中时，应先由退出流程产生 STOP，
 * 再统一清除标志；ARLO 发生后本机已释放总线，退出流程不会乱发 STOP。
 */
static HW_I2C_Status_e HW_I2C_CheckError(I2C_TypeDef *i2c_instance)
{
    /* 仲裁丢失优先级最高，因为此时绝对不能由本机发送 STOP。 */
    if (LL_I2C_IsActiveFlag_ARLO(i2c_instance) != 0U)
    {
        return HW_I2C_STATUS_ARBITRATION_LOST;
    }

    if (LL_I2C_IsActiveFlag_BERR(i2c_instance) != 0U)
    {
        return HW_I2C_STATUS_BUS_ERROR;
    }

    if (LL_I2C_IsActiveFlag_OVR(i2c_instance) != 0U)
    {
        return HW_I2C_STATUS_OVERRUN;
    }

    if (LL_I2C_IsActiveFlag_AF(i2c_instance) != 0U)
    {
        return HW_I2C_STATUS_NACK;
    }

    return HW_I2C_STATUS_OK;
}


/* 清除上一次通信可能留下的错误标志。 */
static void HW_I2C_ClearErrors(I2C_TypeDef *i2c_instance)
{
    if (LL_I2C_IsActiveFlag_AF(i2c_instance) != 0U)
    {
        LL_I2C_ClearFlag_AF(i2c_instance);
    }

    if (LL_I2C_IsActiveFlag_BERR(i2c_instance) != 0U)
    {
        LL_I2C_ClearFlag_BERR(i2c_instance);
    }

    if (LL_I2C_IsActiveFlag_ARLO(i2c_instance) != 0U)
    {
        LL_I2C_ClearFlag_ARLO(i2c_instance);
    }

    if (LL_I2C_IsActiveFlag_OVR(i2c_instance) != 0U)
    {
        LL_I2C_ClearFlag_OVR(i2c_instance);
    }
}


/* 读取指定的 I2C 状态标志。 */
static uint32_t HW_I2C_IsWaitFlagSet(
    I2C_TypeDef *i2c_instance,
    HW_I2C_WaitFlag_e wait_flag)
{
    switch (wait_flag)
    {
        case HW_I2C_WAIT_SB:
            return LL_I2C_IsActiveFlag_SB(i2c_instance);

        case HW_I2C_WAIT_ADDR:
            return LL_I2C_IsActiveFlag_ADDR(i2c_instance);

        case HW_I2C_WAIT_TXE:
            return LL_I2C_IsActiveFlag_TXE(i2c_instance);

        case HW_I2C_WAIT_RXNE:
            return LL_I2C_IsActiveFlag_RXNE(i2c_instance);

        case HW_I2C_WAIT_BTF:
            return LL_I2C_IsActiveFlag_BTF(i2c_instance);

        default:
            return 0U;
    }
}


/* 按指定循环上限等待硬件标志，同时监测 NACK、总线错误等异常。 */
static HW_I2C_Status_e HW_I2C_WaitFlagLimit(
    I2C_TypeDef *i2c_instance,
    HW_I2C_WaitFlag_e wait_flag,
    uint32_t timeout)
{
    HW_I2C_Status_e status;

    while (1)
    {
        /*
         * 即使目标标志已经置位，也要先检查错误。
         * 例如数据被从机 NACK 时，TXE 和 AF 可能同时出现。
         */
        status = HW_I2C_CheckError(i2c_instance);
        if (status != HW_I2C_STATUS_OK)
        {
            return status;
        }

        if (HW_I2C_IsWaitFlagSet(i2c_instance, wait_flag) != 0U)
        {
            return HW_I2C_STATUS_OK;
        }

        if (timeout == 0U)
        {
            return HW_I2C_STATUS_TIMEOUT;
        }

        timeout--;
    }
}


/* 使用普通等待上限等待指定硬件标志。 */
static HW_I2C_Status_e HW_I2C_WaitFlag(
    I2C_TypeDef *i2c_instance,
    HW_I2C_WaitFlag_e wait_flag)
{
    return HW_I2C_WaitFlagLimit(
        i2c_instance,
        wait_flag,
        HW_I2C_WAIT_COUNT);
}


/* 开始新事务前等待 I2C 总线空闲。 */
static HW_I2C_Status_e HW_I2C_WaitBusFree(I2C_TypeDef *i2c_instance)
{
    uint32_t timeout = HW_I2C_WAIT_COUNT;

    while (LL_I2C_IsActiveFlag_BUSY(i2c_instance) != 0U)
    {
        if (timeout == 0U)
        {
            return HW_I2C_STATUS_BUSY;
        }

        timeout--;
    }

    return HW_I2C_STATUS_OK;
}


/*
 * 异常退出时尽量结束当前主机事务，
 * 并把 ACK、POS 和错误标志恢复到下一次通信可用的状态。
 */
static void HW_I2C_AbortTransfer(I2C_TypeDef *i2c_instance)
{
    uint32_t interrupt_state;

    LL_I2C_AcknowledgeNextData(i2c_instance, LL_I2C_NACK);
    LL_I2C_DisableBitPOS(i2c_instance);

    /* 仲裁丢失后总线已经交给其他主机，本机只清错，不能发送 STOP。 */
    if (LL_I2C_IsActiveFlag_ARLO(i2c_instance) != 0U)
    {
        HW_I2C_ClearErrors(i2c_instance);
        return;
    }

    if (LL_I2C_IsActiveFlag_ADDR(i2c_instance) != 0U)
    {
        interrupt_state = __get_PRIMASK();
        __disable_irq();

        LL_I2C_ClearFlag_ADDR(i2c_instance);

        if (LL_I2C_IsActiveFlag_MSL(i2c_instance) != 0U)
        {
            LL_I2C_GenerateStopCondition(i2c_instance);
        }

        if ((interrupt_state & 1U) == 0U)
        {
            __enable_irq();
        }
    }
    else if (LL_I2C_IsActiveFlag_MSL(i2c_instance) != 0U)
    {
        LL_I2C_GenerateStopCondition(i2c_instance);
    }

    HW_I2C_ClearErrors(i2c_instance);
}


/* 产生起始或重复起始信号，并发送 7 位从机地址。 */
static HW_I2C_Status_e HW_I2C_RequestAddress(
    I2C_TypeDef *i2c_instance,
    uint8_t device_address,
    uint8_t read_request)
{
    HW_I2C_Status_e status;
    uint8_t address_byte;

    LL_I2C_GenerateStartCondition(i2c_instance);

    status = HW_I2C_WaitFlag(i2c_instance, HW_I2C_WAIT_SB);
    if (status != HW_I2C_STATUS_OK)
    {
        return status;
    }

    address_byte = (uint8_t)(device_address << 1U);
    if (read_request != 0U)
    {
        address_byte |= 0x01U;
    }

    LL_I2C_TransmitData8(i2c_instance, address_byte);

    return HW_I2C_WaitFlag(i2c_instance, HW_I2C_WAIT_ADDR);
}


/* 执行写阶段；generate_stop 为 0 时保留总线，用于后续重复起始读取。 */
static HW_I2C_Status_e HW_I2C_WritePhase(
    I2C_TypeDef *i2c_instance,
    uint8_t device_address,
    const uint8_t *data,
    uint16_t data_length,
    uint8_t generate_stop)
{
    HW_I2C_Status_e status;
    uint16_t data_index;

    status = HW_I2C_RequestAddress(i2c_instance, device_address, 0U);
    if (status != HW_I2C_STATUS_OK)
    {
        return status;
    }

    LL_I2C_ClearFlag_ADDR(i2c_instance);

    for (data_index = 0U; data_index < data_length; data_index++)
    {
        status = HW_I2C_WaitFlag(i2c_instance, HW_I2C_WAIT_TXE);
        if (status != HW_I2C_STATUS_OK)
        {
            return status;
        }

        LL_I2C_TransmitData8(i2c_instance, data[data_index]);
    }

    status = HW_I2C_WaitFlag(i2c_instance, HW_I2C_WAIT_BTF);
    if (status != HW_I2C_STATUS_OK)
    {
        return status;
    }

    if (generate_stop != 0U)
    {
        LL_I2C_GenerateStopCondition(i2c_instance);
    }

    return HW_I2C_STATUS_OK;
}


/*
 * 执行读取阶段。
 *
 * 经典 I2C 外设对最后 1、2、3 个字节有不同的 ACK、POS、STOP 时序，
 * 因此这里不能只用一个普通循环读取所有字节。
 */
static HW_I2C_Status_e HW_I2C_ReadPhase(
    I2C_TypeDef *i2c_instance,
    uint8_t device_address,
    uint8_t *data,
    uint16_t data_length)
{
    HW_I2C_Status_e status;
    uint16_t remaining = data_length;
    uint16_t data_index = 0U;
    uint32_t interrupt_state;

    LL_I2C_DisableBitPOS(i2c_instance);
    LL_I2C_AcknowledgeNextData(i2c_instance, LL_I2C_ACK);

    status = HW_I2C_RequestAddress(i2c_instance, device_address, 1U);
    if (status != HW_I2C_STATUS_OK)
    {
        goto read_exit;
    }

    if (remaining == 1U)
    {
        LL_I2C_AcknowledgeNextData(i2c_instance, LL_I2C_NACK);

        interrupt_state = __get_PRIMASK();
        __disable_irq();

        LL_I2C_ClearFlag_ADDR(i2c_instance);
        LL_I2C_GenerateStopCondition(i2c_instance);

        if ((interrupt_state & 1U) == 0U)
        {
            __enable_irq();
        }

        status = HW_I2C_WaitFlag(i2c_instance, HW_I2C_WAIT_RXNE);
        if (status == HW_I2C_STATUS_OK)
        {
            data[0] = LL_I2C_ReceiveData8(i2c_instance);
        }

        goto read_exit;
    }

    if (remaining == 2U)
    {
        LL_I2C_EnableBitPOS(i2c_instance);

        interrupt_state = __get_PRIMASK();
        __disable_irq();

        LL_I2C_ClearFlag_ADDR(i2c_instance);
        LL_I2C_AcknowledgeNextData(i2c_instance, LL_I2C_NACK);

        if ((interrupt_state & 1U) == 0U)
        {
            __enable_irq();
        }

        status = HW_I2C_WaitFlag(i2c_instance, HW_I2C_WAIT_BTF);
        if (status != HW_I2C_STATUS_OK)
        {
            goto read_exit;
        }

        interrupt_state = __get_PRIMASK();
        __disable_irq();

        LL_I2C_GenerateStopCondition(i2c_instance);
        data[0] = LL_I2C_ReceiveData8(i2c_instance);

        if ((interrupt_state & 1U) == 0U)
        {
            __enable_irq();
        }

        data[1] = LL_I2C_ReceiveData8(i2c_instance);
        status = HW_I2C_STATUS_OK;
        goto read_exit;
    }

    /* 三个或更多字节：先正常 ACK，最后三个字节使用专用结束时序。 */
    LL_I2C_ClearFlag_ADDR(i2c_instance);

    while (remaining > 3U)
    {
        status = HW_I2C_WaitFlag(i2c_instance, HW_I2C_WAIT_RXNE);
        if (status != HW_I2C_STATUS_OK)
        {
            goto read_exit;
        }

        data[data_index] = LL_I2C_ReceiveData8(i2c_instance);
        data_index++;
        remaining--;
    }

    status = HW_I2C_WaitFlag(i2c_instance, HW_I2C_WAIT_BTF);
    if (status != HW_I2C_STATUS_OK)
    {
        goto read_exit;
    }

    LL_I2C_AcknowledgeNextData(i2c_instance, LL_I2C_NACK);

    interrupt_state = __get_PRIMASK();
    __disable_irq();

    data[data_index] = LL_I2C_ReceiveData8(i2c_instance);
    data_index++;

    status = HW_I2C_WaitFlagLimit(
        i2c_instance,
        HW_I2C_WAIT_BTF,
        HW_I2C_CRITICAL_WAIT_COUNT);
    if (status != HW_I2C_STATUS_OK)
    {
        if ((interrupt_state & 1U) == 0U)
        {
            __enable_irq();
        }

        goto read_exit;
    }

    LL_I2C_GenerateStopCondition(i2c_instance);

    data[data_index] = LL_I2C_ReceiveData8(i2c_instance);
    data_index++;

    if ((interrupt_state & 1U) == 0U)
    {
        __enable_irq();
    }

    data[data_index] = LL_I2C_ReceiveData8(i2c_instance);
    status = HW_I2C_STATUS_OK;


read_exit:
    LL_I2C_DisableBitPOS(i2c_instance);
    LL_I2C_AcknowledgeNextData(i2c_instance, LL_I2C_NACK);

    return status;
}


/* 初始化 I2C 主机、SCL 和 SDA 复用引脚。 */
HW_I2C_Status_e HW_I2C_init(
    I2C_index_e I2C_index,
    I2C_SCL_Pin_e SCL_Pin,
    I2C_SDA_Pin_e SDA_Pin,
    uint32_t ClockSpeed)
{
    const I2C_PinConfig_t *scl_config;
    const I2C_PinConfig_t *sda_config;
    I2C_TypeDef *i2c_instance;
    LL_I2C_InitTypeDef i2c_init = {0};
    LL_RCC_ClocksTypeDef rcc_clocks;
    HW_I2C_Status_e status;

    if (((uint32_t)I2C_index >= (uint32_t)I2C_BUS_COUNT) ||
        ((uint32_t)SCL_Pin >= (uint32_t)SCL_PIN_COUNT) ||
        ((uint32_t)SDA_Pin >= (uint32_t)SDA_PIN_COUNT) ||
        ((ClockSpeed != HW_I2C_STANDARD_CLOCK_SPEED) &&
         (ClockSpeed != HW_I2C_FAST_CLOCK_SPEED)))
    {
        return HW_I2C_STATUS_INVALID_ARG;
    }

    status = HW_I2C_Lock(I2C_index);
    if (status != HW_I2C_STATUS_OK)
    {
        return status;
    }

    /*
     * 运行过程中重新映射同一个 I2C 会让旧引脚继续保持复用状态，
     * 因此本版只允许冷启动初始化一次。
     * 完全相同的重复调用按幂等操作处理，直接返回成功。
     */
    if (s_i2c_active_instance[I2C_index] != NULL)
    {
        if ((s_i2c_active_scl_pin[I2C_index] == SCL_Pin) &&
            (s_i2c_active_sda_pin[I2C_index] == SDA_Pin) &&
            (s_i2c_active_clock_speed[I2C_index] == ClockSpeed))
        {
            status = HW_I2C_STATUS_OK;
        }
        else
        {
            status = HW_I2C_STATUS_ALREADY_INITIALIZED;
        }

        goto init_exit;
    }

    status = HW_I2C_FindPinConfigs(
        I2C_index,
        SCL_Pin,
        SDA_Pin,
        &scl_config,
        &sda_config,
        &i2c_instance);

    if (status != HW_I2C_STATUS_OK)
    {
        goto init_exit;
    }

    status = HW_I2C_EnableClock(i2c_instance);
    if (status != HW_I2C_STATUS_OK)
    {
        goto init_exit;
    }

    /* 先检查时钟条件，避免参数错误时提前改变 GPIO 复用状态。 */
    LL_RCC_GetSystemClocksFreq(&rcc_clocks);

    if (((ClockSpeed == HW_I2C_STANDARD_CLOCK_SPEED) &&
         (rcc_clocks.PCLK1_Frequency < HW_I2C_STANDARD_MODE_MIN_PCLK)) ||
        ((ClockSpeed == HW_I2C_FAST_CLOCK_SPEED) &&
         (rcc_clocks.PCLK1_Frequency < HW_I2C_FAST_MODE_MIN_PCLK)))
    {
        status = HW_I2C_STATUS_INIT_FAILED;
        goto init_exit;
    }

    status = HW_I2C_InitGPIOAlternate(scl_config);
    if (status != HW_I2C_STATUS_OK)
    {
        goto init_exit;
    }

    status = HW_I2C_InitGPIOAlternate(sda_config);
    if (status != HW_I2C_STATUS_OK)
    {
        goto init_exit;
    }

    /*
     * LL_I2C_DeInit 内部会执行 I2C1 强制复位和释放复位。
     * 这一步特意放在 GPIO 复用初始化之后，可避免 PF0/PF1 等组合
     * 在切换为 I2C 复用时残留错误的 BUSY 状态。
     */
    if (LL_I2C_DeInit(i2c_instance) != SUCCESS)
    {
        status = HW_I2C_STATUS_INIT_FAILED;
        goto init_exit;
    }

    i2c_init.ClockSpeed = ClockSpeed;
    i2c_init.DutyCycle = LL_I2C_DUTYCYCLE_2;
    i2c_init.OwnAddress1 = 0U;
    i2c_init.TypeAcknowledge = LL_I2C_NACK;

    if (LL_I2C_Init(i2c_instance, &i2c_init) != SUCCESS)
    {
        status = HW_I2C_STATUS_INIT_FAILED;
        goto init_exit;
    }

    LL_I2C_DisableBitPOS(i2c_instance);
    LL_I2C_AcknowledgeNextData(i2c_instance, LL_I2C_NACK);
    HW_I2C_ClearErrors(i2c_instance);

    s_i2c_active_instance[I2C_index] = i2c_instance;
    s_i2c_active_scl_pin[I2C_index] = SCL_Pin;
    s_i2c_active_sda_pin[I2C_index] = SDA_Pin;
    s_i2c_active_clock_speed[I2C_index] = ClockSpeed;
    status = HW_I2C_STATUS_OK;


init_exit:
    HW_I2C_Unlock(I2C_index);
    return status;
}


/* 通过已经初始化的 I2C 向 7 位地址从机写入数据。 */
HW_I2C_Status_e HW_I2C_Write(
    I2C_index_e I2C_index,
    uint8_t device_address,
    const uint8_t *data,
    uint16_t data_length)
{
    I2C_TypeDef *i2c_instance;
    HW_I2C_Status_e status;

    if (((uint32_t)I2C_index >= (uint32_t)I2C_BUS_COUNT) ||
        (device_address > 0x7FU) ||
        (data == NULL) ||
        (data_length == 0U))
    {
        return HW_I2C_STATUS_INVALID_ARG;
    }

    status = HW_I2C_Lock(I2C_index);
    if (status != HW_I2C_STATUS_OK)
    {
        return status;
    }

    i2c_instance = s_i2c_active_instance[I2C_index];
    if (i2c_instance == NULL)
    {
        status = HW_I2C_STATUS_NOT_READY;
        goto write_exit;
    }

    status = HW_I2C_WaitBusFree(i2c_instance);
    if (status != HW_I2C_STATUS_OK)
    {
        goto write_exit;
    }

    HW_I2C_ClearErrors(i2c_instance);
    LL_I2C_DisableBitPOS(i2c_instance);
    LL_I2C_AcknowledgeNextData(i2c_instance, LL_I2C_NACK);

    status = HW_I2C_WritePhase(
        i2c_instance,
        device_address,
        data,
        data_length,
        1U);

    if (status != HW_I2C_STATUS_OK)
    {
        HW_I2C_AbortTransfer(i2c_instance);
    }


write_exit:
    HW_I2C_Unlock(I2C_index);
    return status;
}


/* 通过已经初始化的 I2C 从 7 位地址从机读取数据。 */
HW_I2C_Status_e HW_I2C_Read(
    I2C_index_e I2C_index,
    uint8_t device_address,
    uint8_t *data,
    uint16_t data_length)
{
    I2C_TypeDef *i2c_instance;
    HW_I2C_Status_e status;

    if (((uint32_t)I2C_index >= (uint32_t)I2C_BUS_COUNT) ||
        (device_address > 0x7FU) ||
        (data == NULL) ||
        (data_length == 0U))
    {
        return HW_I2C_STATUS_INVALID_ARG;
    }

    status = HW_I2C_Lock(I2C_index);
    if (status != HW_I2C_STATUS_OK)
    {
        return status;
    }

    i2c_instance = s_i2c_active_instance[I2C_index];
    if (i2c_instance == NULL)
    {
        status = HW_I2C_STATUS_NOT_READY;
        goto read_public_exit;
    }

    status = HW_I2C_WaitBusFree(i2c_instance);
    if (status != HW_I2C_STATUS_OK)
    {
        goto read_public_exit;
    }

    HW_I2C_ClearErrors(i2c_instance);

    status = HW_I2C_ReadPhase(
        i2c_instance,
        device_address,
        data,
        data_length);

    if (status != HW_I2C_STATUS_OK)
    {
        HW_I2C_AbortTransfer(i2c_instance);
    }


read_public_exit:
    HW_I2C_Unlock(I2C_index);
    return status;
}


/* 先写后读，写阶段结尾不发送 STOP，读取使用重复 START。 */
HW_I2C_Status_e HW_I2C_WriteRead(
    I2C_index_e I2C_index,
    uint8_t device_address,
    const uint8_t *write_data,
    uint16_t write_length,
    uint8_t *read_data,
    uint16_t read_length)
{
    I2C_TypeDef *i2c_instance;
    HW_I2C_Status_e status;

    if (((uint32_t)I2C_index >= (uint32_t)I2C_BUS_COUNT) ||
        (device_address > 0x7FU) ||
        (write_data == NULL) ||
        (write_length == 0U) ||
        (read_data == NULL) ||
        (read_length == 0U))
    {
        return HW_I2C_STATUS_INVALID_ARG;
    }

    status = HW_I2C_Lock(I2C_index);
    if (status != HW_I2C_STATUS_OK)
    {
        return status;
    }

    i2c_instance = s_i2c_active_instance[I2C_index];
    if (i2c_instance == NULL)
    {
        status = HW_I2C_STATUS_NOT_READY;
        goto write_read_exit;
    }

    status = HW_I2C_WaitBusFree(i2c_instance);
    if (status != HW_I2C_STATUS_OK)
    {
        goto write_read_exit;
    }

    HW_I2C_ClearErrors(i2c_instance);
    LL_I2C_DisableBitPOS(i2c_instance);
    LL_I2C_AcknowledgeNextData(i2c_instance, LL_I2C_NACK);

    status = HW_I2C_WritePhase(
        i2c_instance,
        device_address,
        write_data,
        write_length,
        0U);

    if (status == HW_I2C_STATUS_OK)
    {
        status = HW_I2C_ReadPhase(
            i2c_instance,
            device_address,
            read_data,
            read_length);
    }

    if (status != HW_I2C_STATUS_OK)
    {
        HW_I2C_AbortTransfer(i2c_instance);
    }


write_read_exit:
    HW_I2C_Unlock(I2C_index);
    return status;
}
