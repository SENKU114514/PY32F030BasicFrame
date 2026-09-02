#include "./HW_INIT/USART/USART_init.h"


/*
 * GPIO 输出操作示例：
 *
 * 设置 GPIO 输出高电平：
 * LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_11);
 *
 * 设置 GPIO 输出低电平：
 * LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_11);
 *
 * 翻转 GPIO 输出电平：
 * LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_11);
 */


/*
 * UART 硬件层功能：
 *
 * 1. 分别选择合法的 TX 和 RX 引脚；
 * 2. 根据映射表配置 GPIO 复用功能；
 * 3. 配置对应的 USART 外设；
 * 4. 提供串口发送和中断接收功能。
 *
 * 注意：
 * TX 和 RX 引脚分别使用独立的映射表。
 * 只有当 TX、RX 属于同一个 UART 外设时，才允许初始化。
 */


/*
 * 串口发送和接收相关配置。
 *
 * HW_UART_TX_WAIT_COUNT：
 *     发送数据时等待 TXE 或 TC 标志的最大循环次数。
 *
 * HW_UART_RX_BUFFER_SIZE：
 *     UART 接收环形缓冲区大小。
 *
 * HW_UART_RX_BUFFER_MASK：
 *     环形缓冲区索引掩码。
 *     由于使用位与操作实现快速取模，
 *     因此缓冲区大小必须是 2 的幂。
 */
#define HW_UART_TX_WAIT_COUNT 1000000UL
#define HW_UART_RX_BUFFER_SIZE 64U
#define HW_UART_RX_BUFFER_MASK (HW_UART_RX_BUFFER_SIZE - 1U)


/*
 * 检查接收缓冲区大小是否为 2 的幂。
 *
 * 例如：
 *     64  = 0100 0000，满足要求；
 *     32  = 0010 0000，满足要求；
 *     60  = 0011 1100，不满足要求。
 */
#if ((HW_UART_RX_BUFFER_SIZE == 0U) || \
     ((HW_UART_RX_BUFFER_SIZE & HW_UART_RX_BUFFER_MASK) != 0U))
#error "HW_UART_RX_BUFFER_SIZE must be a power of two"
#endif


/*
 * UART TX 或 RX 引脚的通用配置结构体。
 *
 * gpio_port：
 *     GPIO 端口，例如 GPIOA、GPIOB。
 *
 * gpio_pin：
 *     GPIO 引脚掩码，例如 LL_GPIO_PIN_2。
 *
 * gpio_alternate：
 *     GPIO 复用功能编号，例如 LL_GPIO_AF1_USART1。
 */
typedef struct
{
    GPIO_TypeDef *gpio_port;
    uint32_t gpio_pin;
    uint32_t gpio_alternate;
} UART_PinConfig_t;


/*
 * UART TX 引脚映射结构体。
 *
 * 由于 TX 和 RX 使用不同的枚举类型，
 * 因此 TX 和 RX 必须分别使用独立的映射表。
 *
 * uart_index：
 *     UART 逻辑编号，例如 UART1、UART2。
 *
 * pin_index：
 *     TX 引脚逻辑编号，例如 TX_A2、TX_B6。
 *
 * uart_instance：
 *     实际使用的 USART 外设实例，例如 USART1、USART2。
 *
 * pin_config：
 *     TX 引脚对应的 GPIO 端口、引脚和复用功能配置。
 */
typedef struct
{
    UART_index_e uart_index;
    UART_TX_Pin_e pin_index;
    USART_TypeDef *uart_instance;
    UART_PinConfig_t pin_config;
} USART_TX_MAP_t;


/*
 * UART RX 引脚映射结构体。
 *
 * uart_index：
 *     UART 逻辑编号，例如 UART1、UART2。
 *
 * pin_index：
 *     RX 引脚逻辑编号，例如 RX_A3、RX_B7。
 *
 * uart_instance：
 *     实际使用的 USART 外设实例，例如 USART1、USART2。
 *
 * pin_config：
 *     RX 引脚对应的 GPIO 端口、引脚和复用功能配置。
 */
typedef struct
{
    UART_index_e uart_index;
    UART_RX_Pin_e pin_index;
    USART_TypeDef *uart_instance;
    UART_PinConfig_t pin_config;
} USART_RX_MAP_t;


/*
 * 当前芯片的 UART 映射表集合。
 *
 * tx_map：
 *     TX 引脚映射表首地址。
 *
 * tx_map_count：
 *     TX 引脚映射表中的元素数量。
 *
 * rx_map：
 *     RX 引脚映射表首地址。
 *
 * rx_map_count：
 *     RX 引脚映射表中的元素数量。
 *
 * 如果后续支持新的芯片型号，
 * 只需要在下面增加对应芯片的映射表分支。
 */
typedef struct
{
    const USART_TX_MAP_t *tx_map;
    uint32_t tx_map_count;

    const USART_RX_MAP_t *rx_map;
    uint32_t rx_map_count;
} HW_UART_ChipMap_t;


/*
 * UART TX 映射表初始化宏。
 *
 * 参数说明：
 *
 * UART：
 *     UART 逻辑编号。
 *
 * PIN_INDEX：
 *     TX 引脚逻辑编号。
 *
 * INSTANCE：
 *     实际 USART 外设实例。
 *
 * PORT：
 *     GPIO 端口。
 *
 * PIN：
 *     GPIO 引脚掩码。
 *
 * AF：
 *     GPIO 复用功能。
 */
#define UART_TX_MAP(UART, PIN_INDEX, INSTANCE, PORT, PIN, AF) \
    {UART, PIN_INDEX, INSTANCE, {PORT, PIN, AF}}


/*
 * UART RX 映射表初始化宏。
 */
#define UART_RX_MAP(UART, PIN_INDEX, INSTANCE, PORT, PIN, AF) \
    {UART, PIN_INDEX, INSTANCE, {PORT, PIN, AF}}


#if defined(PY32F030PRE)


/*
 * PY32F030 芯片的 UART TX 引脚复用映射表。
 *
 * 每一项表示：
 *
 *     某个 UART 外设可以使用哪个 GPIO 引脚作为 TX，
 *     以及该引脚对应的 GPIO 复用功能。
 *
 * 同一个 GPIO 引脚可能根据不同的复用配置连接到不同的 UART 外设。
 */


/* PY32F030 TX 引脚复用映射表 */
static const USART_TX_MAP_t USART_TX_MAP[] =
{
#if defined(USART2) && defined(GPIOA)
    /* USART2_TX -> PA0，复用功能 AF9 */
    UART_TX_MAP(UART2, TX_A0, USART2, GPIOA, LL_GPIO_PIN_0, LL_GPIO_AF9_USART2),
#endif

#if defined(USART1) && defined(GPIOA)
    /* USART1_TX -> PA2，复用功能 AF1 */
    UART_TX_MAP(UART1, TX_A2, USART1, GPIOA, LL_GPIO_PIN_2, LL_GPIO_AF1_USART1),
#endif

#if defined(USART2) && defined(GPIOA)
    /* USART2_TX -> PA2，复用功能 AF4 */
    UART_TX_MAP(UART2, TX_A2, USART2, GPIOA, LL_GPIO_PIN_2, LL_GPIO_AF4_USART2),

    /* USART2_TX -> PA4，复用功能 AF9 */
    UART_TX_MAP(UART2, TX_A4, USART2, GPIOA, LL_GPIO_PIN_4, LL_GPIO_AF9_USART2),
#endif

#if defined(USART1) && defined(GPIOA)
    /* USART1_TX -> PA7，复用功能 AF8 */
    UART_TX_MAP(UART1, TX_A7, USART1, GPIOA, LL_GPIO_PIN_7, LL_GPIO_AF8_USART1),
#endif

#if defined(USART2) && defined(GPIOA)
    /* USART2_TX -> PA7，复用功能 AF9 */
    UART_TX_MAP(UART2, TX_A7, USART2, GPIOA, LL_GPIO_PIN_7, LL_GPIO_AF9_USART2),
#endif

#if defined(USART1) && defined(GPIOA)
    /* USART1_TX -> PA9，复用功能 AF1 */
    UART_TX_MAP(UART1, TX_A9, USART1, GPIOA, LL_GPIO_PIN_9, LL_GPIO_AF1_USART1),

    /* USART1_TX -> PA10，复用功能 AF8 */
    UART_TX_MAP(UART1, TX_A10, USART1, GPIOA, LL_GPIO_PIN_10, LL_GPIO_AF8_USART1),

    /* USART1_TX -> PA14，复用功能 AF1 */
    UART_TX_MAP(UART1, TX_A14, USART1, GPIOA, LL_GPIO_PIN_14, LL_GPIO_AF1_USART1),
#endif

#if defined(USART2) && defined(GPIOA)
    /* USART2_TX -> PA9，复用功能 AF4 */
    UART_TX_MAP(UART2, TX_A9, USART2, GPIOA, LL_GPIO_PIN_9, LL_GPIO_AF4_USART2),

    /* USART2_TX -> PA14，复用功能 AF4 */
    UART_TX_MAP(UART2, TX_A14, USART2, GPIOA, LL_GPIO_PIN_14, LL_GPIO_AF4_USART2),
#endif

#if defined(USART1) && defined(GPIOB)
    /* USART1_TX -> PB6，复用功能 AF0 */
    UART_TX_MAP(UART1, TX_B6, USART1, GPIOB, LL_GPIO_PIN_6, LL_GPIO_AF0_USART1),

    /* USART1_TX -> PB8，复用功能 AF8 */
    UART_TX_MAP(UART1, TX_B8, USART1, GPIOB, LL_GPIO_PIN_8, LL_GPIO_AF8_USART1),
#endif

#if defined(USART2) && defined(GPIOB)
    /* USART2_TX -> PB6，复用功能 AF4 */
    UART_TX_MAP(UART2, TX_B6, USART2, GPIOB, LL_GPIO_PIN_6, LL_GPIO_AF4_USART2),

    /* USART2_TX -> PB8，复用功能 AF4 */
    UART_TX_MAP(UART2, TX_B8, USART2, GPIOB, LL_GPIO_PIN_8, LL_GPIO_AF4_USART2),
#endif

#if defined(USART2) && defined(GPIOF)
    /* USART2_TX -> PF0，复用功能 AF9 */
    UART_TX_MAP(UART2, TX_F0, USART2, GPIOF, LL_GPIO_PIN_0, LL_GPIO_AF9_USART2),
#endif

#if defined(USART1) && defined(GPIOF)
    /* USART1_TX -> PF1，复用功能 AF8 */
    UART_TX_MAP(UART1, TX_F1, USART1, GPIOF, LL_GPIO_PIN_1, LL_GPIO_AF8_USART1),

    /* USART1_TX -> PF3，复用功能 AF0 */
    UART_TX_MAP(UART1, TX_F3, USART1, GPIOF, LL_GPIO_PIN_3, LL_GPIO_AF0_USART1),
#endif

#if defined(USART2) && defined(GPIOF)
    /* USART2_TX -> PF1，复用功能 AF4 */
    UART_TX_MAP(UART2, TX_F1, USART2, GPIOF, LL_GPIO_PIN_1, LL_GPIO_AF4_USART2),

    /* USART2_TX -> PF3，复用功能 AF4 */
    UART_TX_MAP(UART2, TX_F3, USART2, GPIOF, LL_GPIO_PIN_3, LL_GPIO_AF4_USART2),
#endif
};


/*
 * PY32F030 芯片的 UART RX 引脚复用映射表。
 *
 * 每一项表示：
 *
 *     某个 UART 外设可以使用哪个 GPIO 引脚作为 RX，
 *     以及该引脚对应的 GPIO 复用功能。
 */


/* PY32F030 RX 引脚复用映射表 */
static const USART_RX_MAP_t USART_RX_MAP[] =
{
#if defined(USART2) && defined(GPIOA)
    /* USART2_RX -> PA1，复用功能 AF9 */
    UART_RX_MAP(UART2, RX_A1, USART2, GPIOA, LL_GPIO_PIN_1, LL_GPIO_AF9_USART2),
#endif

#if defined(USART1) && defined(GPIOA)
    /* USART1_RX -> PA3，复用功能 AF1 */
    UART_RX_MAP(UART1, RX_A3, USART1, GPIOA, LL_GPIO_PIN_3, LL_GPIO_AF1_USART1),
#endif

#if defined(USART2) && defined(GPIOA)
    /* USART2_RX -> PA3，复用功能 AF4 */
    UART_RX_MAP(UART2, RX_A3, USART2, GPIOA, LL_GPIO_PIN_3, LL_GPIO_AF4_USART2),

    /* USART2_RX -> PA5，复用功能 AF9 */
    UART_RX_MAP(UART2, RX_A5, USART2, GPIOA, LL_GPIO_PIN_5, LL_GPIO_AF9_USART2),
#endif

#if defined(USART1) && defined(GPIOA)
    /* USART1_RX -> PA8，复用功能 AF8 */
    UART_RX_MAP(UART1, RX_A8, USART1, GPIOA, LL_GPIO_PIN_8, LL_GPIO_AF8_USART1),

    /* USART1_RX -> PA9，复用功能 AF8 */
    UART_RX_MAP(UART1, RX_A9, USART1, GPIOA, LL_GPIO_PIN_9, LL_GPIO_AF8_USART1),

    /* USART1_RX -> PA10，复用功能 AF1 */
    UART_RX_MAP(UART1, RX_A10, USART1, GPIOA, LL_GPIO_PIN_10, LL_GPIO_AF1_USART1),

    /* USART1_RX -> PA13，复用功能 AF8 */
    UART_RX_MAP(UART1, RX_A13, USART1, GPIOA, LL_GPIO_PIN_13, LL_GPIO_AF8_USART1),

    /* USART1_RX -> PA15，复用功能 AF1 */
    UART_RX_MAP(UART1, RX_A15, USART1, GPIOA, LL_GPIO_PIN_15, LL_GPIO_AF1_USART1),
#endif

#if defined(USART2) && defined(GPIOA)
    /* USART2_RX -> PA8，复用功能 AF9 */
    UART_RX_MAP(UART2, RX_A8, USART2, GPIOA, LL_GPIO_PIN_8, LL_GPIO_AF9_USART2),

    /* USART2_RX -> PA10，复用功能 AF4 */
    UART_RX_MAP(UART2, RX_A10, USART2, GPIOA, LL_GPIO_PIN_10, LL_GPIO_AF4_USART2),

    /* USART2_RX -> PA15，复用功能 AF4 */
    UART_RX_MAP(UART2, RX_A15, USART2, GPIOA, LL_GPIO_PIN_15, LL_GPIO_AF4_USART2),
#endif

#if defined(USART1) && defined(GPIOB)
    /* USART1_RX -> PB2，复用功能 AF0 */
    UART_RX_MAP(UART1, RX_B2, USART1, GPIOB, LL_GPIO_PIN_2, LL_GPIO_AF0_USART1),

    /* USART1_RX -> PB7，复用功能 AF0 */
    UART_RX_MAP(UART1, RX_B7, USART1, GPIOB, LL_GPIO_PIN_7, LL_GPIO_AF0_USART1),
#endif

#if defined(USART2) && defined(GPIOB)
    /* USART2_RX -> PB2，复用功能 AF3 */
    UART_RX_MAP(UART2, RX_B2, USART2, GPIOB, LL_GPIO_PIN_2, LL_GPIO_AF3_USART2),

    /* USART2_RX -> PB7，复用功能 AF4 */
    UART_RX_MAP(UART2, RX_B7, USART2, GPIOB, LL_GPIO_PIN_7, LL_GPIO_AF4_USART2),
#endif

#if defined(USART1) && defined(GPIOF)
    /* USART1_RX -> PF0，复用功能 AF8 */
    UART_RX_MAP(UART1, RX_F0, USART1, GPIOF, LL_GPIO_PIN_0, LL_GPIO_AF8_USART1),
#endif

#if defined(USART2) && defined(GPIOF)
    /* USART2_RX -> PF0，复用功能 AF4 */
    UART_RX_MAP(UART2, RX_F0, USART2, GPIOF, LL_GPIO_PIN_0, LL_GPIO_AF4_USART2),

    /* USART2_RX -> PF1，复用功能 AF9 */
    UART_RX_MAP(UART2, RX_F1, USART2, GPIOF, LL_GPIO_PIN_1, LL_GPIO_AF9_USART2),

    /* USART2_RX -> PF2，复用功能 AF4 */
    UART_RX_MAP(UART2, RX_F2, USART2, GPIOF, LL_GPIO_PIN_2, LL_GPIO_AF4_USART2),
#endif
};


/*
 * 当前芯片的 UART 映射表。
 *
 * sizeof(USART_TX_MAP) / sizeof(USART_TX_MAP[0])：
 *     计算 TX 映射表的元素数量。
 *
 * sizeof(USART_RX_MAP) / sizeof(USART_RX_MAP[0])：
 *     计算 RX 映射表的元素数量。
 */
static const HW_UART_ChipMap_t s_uart_chip_map =
{
    USART_TX_MAP,
    sizeof(USART_TX_MAP) / sizeof(USART_TX_MAP[0]),

    USART_RX_MAP,
    sizeof(USART_RX_MAP) / sizeof(USART_RX_MAP[0])
};


#elif defined(PY32F002BPRE)


/*
 * PY32F002B 芯片扩展位置。
 *
 * 当前暂未添加经过确认的 TX/RX 引脚映射表。
 * 后续支持该芯片时，可以在此处添加对应的映射表。
 */
static const HW_UART_ChipMap_t s_uart_chip_map =
{
    NULL,
    0U,
    NULL,
    0U
};


#else


/*
 * 当前芯片型号暂未添加经过确认的 UART 引脚映射表。
 */
static const HW_UART_ChipMap_t s_uart_chip_map =
{
    NULL,
    0U,
    NULL,
    0U
};


#endif


/*
 * 映射表初始化宏只在当前文件中使用，
 * 使用完毕后取消定义，避免宏名称污染其他代码。
 */
#undef UART_TX_MAP
#undef UART_RX_MAP


/*
 * 保存每个 UART 当前正在使用的 USART 外设实例。
 *
 * 如果对应元素为 NULL，表示该 UART 尚未初始化或当前不可用。
 *
 * 例如：
 *     s_uart_active_instance[UART1] = USART1;
 */
static USART_TypeDef *s_uart_active_instance[UART_COUNT] =
{
    NULL
};


/*
 * UART 接收环形缓冲区。
 *
 * data：
 *     实际保存接收数据的数组。
 *
 * write_index：
 *     写入位置。
 *     由 UART 中断服务程序更新。
 *
 * read_index：
 *     读取位置。
 *     由应用程序读取数据时更新。
 *
 * overflow：
 *     缓冲区溢出标志。
 *     当接收数据速度大于应用程序读取速度时，
 *     新接收的数据会被丢弃，并设置该标志。
 */
typedef struct
{
    uint8_t data[HW_UART_RX_BUFFER_SIZE];

    volatile uint16_t write_index;
    volatile uint16_t read_index;

    volatile uint8_t overflow;
} HW_UART_RxBuffer_t;


/*
 * 每个 UART 对应一个独立的接收环形缓冲区。
 */
static HW_UART_RxBuffer_t s_uart_rx_buffer[UART_COUNT];


/*
 * 一次性接收状态。
 *
 * data/capacity：本轮由调用者提供的接收数组及其容量。
 * length：本轮已经保存的字节数。
 * activity_count：本轮实际到达的字节计数；即使数组已满也继续递增，
 *                 用于正确判断总线是否已经连续静默。
 * active：只有该标志为 1 时，中断才会把字节写入本轮数组。
 * overflow：对端回复超过调用者给出的数组容量。
 * rx_error：本轮出现 ORE、FE 或 NE 接收错误。
 */
typedef struct
{
    uint8_t *data;
    uint16_t capacity;

    volatile uint16_t length;
    volatile uint32_t activity_count;

    volatile uint8_t active;
    volatile uint8_t overflow;
    volatile uint8_t rx_error;
} HW_UART_RxOnce_t;


/* 每个 UART 各自拥有一份互不影响的一次性接收状态。 */
static HW_UART_RxOnce_t s_uart_rx_once[UART_COUNT];


/*
 * 根据 UART 编号分别查找 TX 和 RX 引脚配置。
 *
 * TX 和 RX 分开查找的原因：
 *     TX 和 RX 的合法引脚集合不同，
 *     因此不能使用同一个映射表。
 *
 * 函数同时检查：
 *     1. UART 编号是否匹配；
 *     2. TX 引脚是否存在；
 *     3. RX 引脚是否存在；
 *     4. TX 和 RX 是否属于同一个 USART 外设。
 */
static HW_UART_Status_e HW_UART_FindPinConfigs(
    UART_index_e UART_index,
    UART_TX_Pin_e TX_Pin,
    UART_RX_Pin_e RX_Pin,
    const UART_PinConfig_t **tx_config,
    const UART_PinConfig_t **rx_config,
    USART_TypeDef **uart_instance)
{
    uint32_t index;
    const USART_TX_MAP_t *tx_map = NULL;
    const USART_RX_MAP_t *rx_map = NULL;


    /*
     * 检查输出参数指针是否为空。
     */
    if ((tx_config == NULL) ||
        (rx_config == NULL) ||
        (uart_instance == NULL))
    {
        return HW_UART_STATUS_INVALID_ARG;
    }


    /*
     * 在 TX 映射表中查找指定 UART 和 TX 引脚。
     */
    for (index = 0U;
         index < s_uart_chip_map.tx_map_count;
         index++)
    {
        if ((s_uart_chip_map.tx_map[index].uart_index == UART_index) &&
            (s_uart_chip_map.tx_map[index].pin_index == TX_Pin))
        {
            tx_map = &s_uart_chip_map.tx_map[index];
            break;
        }
    }


    /*
     * 在 RX 映射表中查找指定 UART 和 RX 引脚。
     */
    for (index = 0U;
         index < s_uart_chip_map.rx_map_count;
         index++)
    {
        if ((s_uart_chip_map.rx_map[index].uart_index == UART_index) &&
            (s_uart_chip_map.rx_map[index].pin_index == RX_Pin))
        {
            rx_map = &s_uart_chip_map.rx_map[index];
            break;
        }
    }


    /*
     * 以下任意一种情况都会认为配置不支持：
     *
     * 1. 没有找到 TX 配置；
     * 2. 没有找到 RX 配置；
     * 3. TX 和 RX 对应的 USART 外设不同。
     */
    if ((tx_map == NULL) ||
        (rx_map == NULL) ||
        (tx_map->uart_instance != rx_map->uart_instance))
    {
        return HW_UART_STATUS_UNSUPPORTED;
    }


    /*
     * 返回找到的 TX、RX 引脚配置以及 USART 外设实例。
     */
    *tx_config = &tx_map->pin_config;
    *rx_config = &rx_map->pin_config;
    *uart_instance = tx_map->uart_instance;

    return HW_UART_STATUS_OK;
}


/*
 * 使能指定 USART 外设对应的总线时钟。
 *
 * 不同 USART 外设可能位于不同的 APB 总线上，
 * 因此需要根据实际的 USART 实例调用对应的时钟使能函数。
 */
static HW_UART_Status_e HW_UART_EnableClock(
    USART_TypeDef *uart_instance)
{
#ifdef USART1

    /*
     * USART1 位于 APB2 总线。
     */
    if (uart_instance == USART1)
    {
        LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1);
        return HW_UART_STATUS_OK;
    }

#endif


#ifdef USART2

    /*
     * USART2 位于 APB1 总线。
     */
    if (uart_instance == USART2)
    {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
        return HW_UART_STATUS_OK;
    }

#endif


    /*
     * 传入的 USART 外设未被当前芯片支持。
     */
    return HW_UART_STATUS_UNSUPPORTED;
}


/*
 * 使能 UART 引脚所在 GPIO 端口的时钟。
 */
static HW_UART_Status_e HW_UART_EnableGPIOClock(
    GPIO_TypeDef *gpio_port)
{
    /*
     * GPIO 端口指针为空，说明参数无效。
     */
    if (gpio_port == NULL)
    {
        return HW_UART_STATUS_INVALID_ARG;
    }


#ifdef GPIOA

    /*
     * 使能 GPIOA 时钟。
     */
    if (gpio_port == GPIOA)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
        return HW_UART_STATUS_OK;
    }

#endif


#ifdef GPIOB

    /*
     * 使能 GPIOB 时钟。
     */
    if (gpio_port == GPIOB)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
        return HW_UART_STATUS_OK;
    }

#endif


#ifdef GPIOC

    /*
     * 使能 GPIOC 时钟。
     */
    if (gpio_port == GPIOC)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
        return HW_UART_STATUS_OK;
    }

#endif


#ifdef GPIOF

    /*
     * 使能 GPIOF 时钟。
     */
    if (gpio_port == GPIOF)
    {
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);
        return HW_UART_STATUS_OK;
    }

#endif


    /*
     * 传入的 GPIO 端口未被当前芯片支持。
     */
    return HW_UART_STATUS_UNSUPPORTED;
}


/*
 * 初始化一个 UART 复用功能 GPIO 引脚。
 *
 * 初始化内容包括：
 *
 * 1. 使能 GPIO 端口时钟；
 * 2. 设置 GPIO 为复用模式；
 * 3. 设置 GPIO 输出速度；
 * 4. 设置推挽输出；
 * 5. 设置上拉；
 * 6. 设置对应的复用功能。
 */
static HW_UART_Status_e HW_UART_InitGPIOAlternate(
    const UART_PinConfig_t *pin_config)
{
    LL_GPIO_InitTypeDef gpio_init = {0};
    HW_UART_Status_e status;


    /*
     * 检查引脚配置指针是否为空。
     */
    if (pin_config == NULL)
    {
        return HW_UART_STATUS_INVALID_ARG;
    }


    /*
     * 使能该引脚所在 GPIO 端口的时钟。
     */
    status = HW_UART_EnableGPIOClock(pin_config->gpio_port);
    if (status != HW_UART_STATUS_OK)
    {
        return status;
    }


    /*
     * 配置 GPIO 复用参数。
     */
    gpio_init.Pin = pin_config->gpio_pin;
    gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = LL_GPIO_PULL_UP;
    gpio_init.Alternate = pin_config->gpio_alternate;


    /*
     * 调用 LL 库初始化 GPIO。
     */
    if (LL_GPIO_Init(pin_config->gpio_port, &gpio_init) != SUCCESS)
    {
        return HW_UART_STATUS_GPIO_INIT_FAILED;
    }

    return HW_UART_STATUS_OK;
}


/*
 * 清空指定 UART 的接收缓冲区。
 *
 * 通常在 UART 初始化完成后调用，
 * 确保接收缓冲区中没有残留数据。
 */
static void HW_UART_ResetRxBuffer(UART_index_e UART_index)
{
    s_uart_rx_buffer[UART_index].write_index = 0U;
    s_uart_rx_buffer[UART_index].read_index = 0U;
    s_uart_rx_buffer[UART_index].overflow = 0U;
}


/* 清空指定 UART 的一次性接收状态。 */
static void HW_UART_ResetRxOnce(UART_index_e UART_index)
{
    HW_UART_RxOnce_t *rx_once = &s_uart_rx_once[UART_index];

    rx_once->data = NULL;
    rx_once->capacity = 0U;
    rx_once->length = 0U;
    rx_once->activity_count = 0U;
    rx_once->active = 0U;
    rx_once->overflow = 0U;
    rx_once->rx_error = 0U;
}


/*
 * 向指定 UART 的接收环形缓冲区中写入一个字节。
 *
 * 当缓冲区已满时：
 *
 * 1. 保留缓冲区中已有的数据；
 * 2. 丢弃当前新接收到的字节；
 * 3. 设置溢出标志。
 */
static void HW_UART_PushRxByte(UART_index_e UART_index,
                                uint8_t data)
{
    HW_UART_RxBuffer_t *rx_buffer =
        &s_uart_rx_buffer[UART_index];

    uint16_t used_length;


    /*
     * 计算当前缓冲区中尚未读取的数据长度。
     *
     * write_index 和 read_index 使用无符号整数，
     * 即使索引发生回绕，减法结果仍可用于计算已使用空间。
     */
    used_length =
        (uint16_t)(rx_buffer->write_index -
                   rx_buffer->read_index);


    /*
     * 缓冲区已满。
     */
    if (used_length >= HW_UART_RX_BUFFER_SIZE)
    {
        rx_buffer->overflow = 1U;
        return;
    }


    /*
     * 使用掩码实现环形缓冲区索引回绕。
     *
     * 因为 HW_UART_RX_BUFFER_SIZE 必须是 2 的幂，
     * 所以：
     *
     *     index % BUFFER_SIZE
     *
     * 可以使用：
     *
     *     index & BUFFER_MASK
     *
     * 替代。
     */
    rx_buffer->data[
        rx_buffer->write_index & HW_UART_RX_BUFFER_MASK] = data;

    rx_buffer->write_index++;
}


/*
 * 把一个新字节写入当前的一次性接收数组。
 *
 * 一次性接收未开启时直接返回；数组已满时不覆盖已有内容，
 * 只记录溢出并继续累计活动次数，供接收函数判断后续静默时间。
 */
static void HW_UART_PushRxOnceByte(UART_index_e UART_index,
                                    uint8_t data,
                                    uint8_t receive_error)
{
    HW_UART_RxOnce_t *rx_once = &s_uart_rx_once[UART_index];

    if (rx_once->active == 0U)
    {
        return;
    }

    rx_once->activity_count++;

    if (rx_once->length < rx_once->capacity)
    {
        rx_once->data[rx_once->length] = data;
        rx_once->length++;
    }
    else
    {
        rx_once->overflow = 1U;
    }

    if (receive_error != 0U)
    {
        rx_once->rx_error = 1U;
    }
}


/*
 * UART 接收中断处理函数。
 *
 * 处理内容包括：
 *
 * 1. 检查接收数据是否就绪；
 * 2. 读取接收到的字节；
 * 3. 将数据保存到对应 UART 的接收缓冲区；
 * 4. 检查并处理 ORE、FE、NE 等接收错误。
 */
static void HW_UART_ReceiveIRQHandler(
    UART_index_e UART_index,
    USART_TypeDef *uart_instance)
{
    uint8_t rx_data;
    uint8_t receive_error;


    /*
     * 检查 UART 接收错误标志：
     *
     * ORE：过载错误；
     * FE ：帧错误；
     * NE ：噪声错误。
     */
    receive_error =
        (uint8_t)(((LL_USART_IsActiveFlag_ORE(uart_instance) != 0U) ||
                   (LL_USART_IsActiveFlag_FE(uart_instance) != 0U) ||
                   (LL_USART_IsActiveFlag_NE(uart_instance) != 0U))
                  ? 1U
                  : 0U);


    /*
     * 检查接收数据寄存器是否有新数据。
     */
    if (LL_USART_IsActiveFlag_RXNE(uart_instance) != 0U)
    {
        /*
         * 读取接收数据。
         *
         * 读取数据寄存器通常会清除 RXNE 标志，
         * 同时清除当前接收错误状态。
         */
        rx_data = LL_USART_ReceiveData8(uart_instance);


        /*
         * 只有当该 UART 当前确实绑定了这个 USART 实例时，
         * 才将接收到的数据写入对应的环形缓冲区。
         */
        if (s_uart_active_instance[UART_index] == uart_instance)
        {
            if (s_uart_rx_once[UART_index].active != 0U)
            {
                /* 一次性窗口拥有本字节，不再复制到环形缓冲。 */
                HW_UART_PushRxOnceByte(UART_index,
                                       rx_data,
                                       receive_error);
            }
            else
            {
                HW_UART_PushRxByte(UART_index, rx_data);
            }

            /*
             * 如果读取数据前检测到了接收错误，
             * 使用 overflow 标志向上层报告异常。
             */
            if (receive_error != 0U)
            {
                if (s_uart_rx_once[UART_index].active == 0U)
                {
                    s_uart_rx_buffer[UART_index].overflow = 1U;
                }
            }
        }
    }
    else if (receive_error != 0U)
    {
        /*
         * 没有接收数据，但存在接收错误。
         *
         * 通过清除 ORE 标志完成错误状态处理。
         * 根据芯片手册，读取状态寄存器和数据寄存器
         * 通常可以共同清除 ORE、FE 和 NE 标志。
         */
        LL_USART_ClearFlag_ORE(uart_instance);

        if (s_uart_active_instance[UART_index] == uart_instance)
        {
            if (s_uart_rx_once[UART_index].active != 0U)
            {
                s_uart_rx_once[UART_index].rx_error = 1U;
            }
            else
            {
                s_uart_rx_buffer[UART_index].overflow = 1U;
            }
        }
    }
}


/*
 * 使能指定 UART 的接收中断，
 * 同时配置对应的 NVIC 中断通道。
 *
 * 使能的中断包括：
 *
 * 1. RXNE：接收数据寄存器非空中断；
 * 2. ERROR：接收错误中断。
 */
static HW_UART_Status_e HW_UART_EnableReceiveIT(
    UART_index_e UART_index,
    USART_TypeDef *uart_instance)
{
#ifdef USART1

    /*
     * 配置 USART1 接收中断。
     */
    if ((UART_index == UART1) &&
        (uart_instance == USART1))
    {
        NVIC_ClearPendingIRQ(USART1_IRQn);
        NVIC_SetPriority(USART1_IRQn, 1U);

        LL_USART_EnableIT_ERROR(USART1);
        LL_USART_EnableIT_RXNE(USART1);

        NVIC_EnableIRQ(USART1_IRQn);

        return HW_UART_STATUS_OK;
    }

#endif


#ifdef USART2

    /*
     * 配置 USART2 接收中断。
     */
    if ((UART_index == UART2) &&
        (uart_instance == USART2))
    {
        NVIC_ClearPendingIRQ(USART2_IRQn);
        NVIC_SetPriority(USART2_IRQn, 1U);

        LL_USART_EnableIT_ERROR(USART2);
        LL_USART_EnableIT_RXNE(USART2);

        NVIC_EnableIRQ(USART2_IRQn);

        return HW_UART_STATUS_OK;
    }

#endif


    /*
     * 当前 UART 外设或中断通道不受支持。
     */
    return HW_UART_STATUS_UNSUPPORTED;
}


#ifdef USART1

/*
 * USART1 中断向量入口。
 *
 * 当 USART1 产生接收数据或接收错误中断时，
 * 由该函数进入统一的 UART 接收中断处理函数。
 */
void USART1_IRQHandler(void)
{
    HW_UART_ReceiveIRQHandler(UART1, USART1);
}

#endif


#ifdef USART2

/*
 * USART2 中断向量入口。
 *
 * 当 USART2 产生接收数据或接收错误中断时，
 * 由该函数进入统一的 UART 接收中断处理函数。
 */
void USART2_IRQHandler(void)
{
    HW_UART_ReceiveIRQHandler(UART2, USART2);
}

#endif


/*
 * 等待 UART 状态标志。
 *
 * wait_transfer_complete 为 0：
 *     等待 TXE 标志，表示发送数据寄存器为空，
 *     可以写入下一个待发送字节。
 *
 * wait_transfer_complete 不为 0：
 *     等待 TC 标志，表示最后一个字节已经完整发送完成。
 *
 * 函数使用循环次数作为超时判断，
 * 不依赖系统时钟或系统定时器。
 */
static HW_UART_Status_e HW_UART_WaitFlag(
    USART_TypeDef *uart_instance,
    uint8_t wait_transfer_complete)
{
    uint32_t timeout = HW_UART_TX_WAIT_COUNT;


    /*
     * 根据参数选择等待 TXE 或 TC 标志。
     */
    while ((wait_transfer_complete != 0U)
               ? (LL_USART_IsActiveFlag_TC(uart_instance) == 0U)
               : (LL_USART_IsActiveFlag_TXE(uart_instance) == 0U))
    {
        /*
         * 超时退出，避免因硬件异常导致程序永久阻塞。
         */
        if (timeout == 0U)
        {
            return HW_UART_STATUS_TIMEOUT;
        }

        timeout--;
    }

    return HW_UART_STATUS_OK;
}


/*
 * 初始化一个 UART。
 *
 * 参数：
 *
 * UART_index：
 *     UART 逻辑编号，例如 UART1、UART2。
 *
 * TX_Pin：
 *     选择的 TX 引脚逻辑编号。
 *
 * RX_Pin：
 *     选择的 RX 引脚逻辑编号。
 *
 * BaudRate：
 *     UART 波特率，例如 115200。
 *
 * 初始化过程：
 *
 * 1. 检查 UART 编号和波特率；
 * 2. 查找合法的 TX/RX 引脚配置；
 * 3. 检查 TX/RX 是否属于同一个 USART 外设；
 * 4. 使能 USART 外设时钟；
 * 5. 初始化 TX GPIO 复用功能；
 * 6. 初始化 RX GPIO 复用功能；
 * 7. 配置 USART 参数；
 * 8. 使能 USART；
 * 9. 清空接收缓冲区；
 * 10. 开启接收中断。
 */
HW_UART_Status_e HW_UART_init(
    UART_index_e UART_index,
    UART_TX_Pin_e TX_Pin,
    UART_RX_Pin_e RX_Pin,
    uint32_t BaudRate)
{
    const UART_PinConfig_t *tx_config;
    const UART_PinConfig_t *rx_config;
    USART_TypeDef *uart_instance;
    HW_UART_Status_e status;
    LL_USART_InitTypeDef uart_init = {0};


    /*
     * 检查 UART 编号是否越界，以及波特率是否为 0。
     */
    if (((uint32_t)UART_index >= (uint32_t)UART_COUNT) ||
        (BaudRate == 0U))
    {
        return HW_UART_STATUS_INVALID_ARG;
    }


    /*
     * 初始化前先清除该 UART 的当前外设绑定状态。
     *
     * 这样可以避免初始化失败时，
     * 其他代码误认为该 UART 仍处于可用状态。
     */
    s_uart_active_instance[UART_index] = NULL;


    /*
     * 查找 TX/RX 引脚配置和对应的 USART 外设实例。
     */
    status = HW_UART_FindPinConfigs(
        UART_index,
        TX_Pin,
        RX_Pin,
        &tx_config,
        &rx_config,
        &uart_instance);

    if (status != HW_UART_STATUS_OK)
    {
        return status;
    }


    /*
     * 使能 USART 外设时钟。
     */
    status = HW_UART_EnableClock(uart_instance);
    if (status != HW_UART_STATUS_OK)
    {
        return status;
    }


    /*
     * 初始化 TX GPIO 复用功能。
     */
    status = HW_UART_InitGPIOAlternate(tx_config);
    if (status != HW_UART_STATUS_OK)
    {
        return status;
    }


    /*
     * 初始化 RX GPIO 复用功能。
     */
    status = HW_UART_InitGPIOAlternate(rx_config);
    if (status != HW_UART_STATUS_OK)
    {
        return status;
    }


    /*
     * 在重新配置 USART 前，先关闭：
     *
     * 1. RXNE 接收中断；
     * 2. UART 错误中断；
     * 3. USART 外设。
     */
    LL_USART_DisableIT_RXNE(uart_instance);
    LL_USART_DisableIT_ERROR(uart_instance);
    LL_USART_Disable(uart_instance);


    /*
     * 配置 USART 参数。
     */
    uart_init.BaudRate = BaudRate;

    /* 8 位数据宽度 */
    uart_init.DataWidth = LL_USART_DATAWIDTH_8B;

    /* 1 位停止位 */
    uart_init.StopBits = LL_USART_STOPBITS_1;

    /* 无奇偶校验 */
    uart_init.Parity = LL_USART_PARITY_NONE;

    /* 同时使能发送和接收 */
    uart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;

    /* 不使用硬件流控 */
    uart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;

    /* 16 倍过采样 */
    uart_init.OverSampling = LL_USART_OVERSAMPLING_16;


    /*
     * 调用 LL 库初始化 USART。
     */
    if (LL_USART_Init(uart_instance, &uart_init) != SUCCESS)
    {
        return HW_UART_STATUS_INIT_FAILED;
    }


    /*
     * 配置 USART 异步通信模式。
     */
    LL_USART_ConfigAsyncMode(uart_instance);


    /*
     * 使能 USART 外设。
     */
    LL_USART_Enable(uart_instance);


    /*
     * 清空该 UART 的接收缓冲区。
     */
    HW_UART_ResetRxBuffer(UART_index);
    HW_UART_ResetRxOnce(UART_index);


    /*
     * 保存当前 UART 实际使用的 USART 外设实例。
     *
     * 中断处理函数会通过该变量判断当前 UART 是否处于有效状态。
     */
    s_uart_active_instance[UART_index] = uart_instance;


    /*
     * 开启 UART 接收中断和对应的 NVIC 中断通道。
     */
    status = HW_UART_EnableReceiveIT(UART_index, uart_instance);
    if (status != HW_UART_STATUS_OK)
    {
        /*
         * 如果开启中断失败，则撤销当前 UART 的有效状态。
         */
        s_uart_active_instance[UART_index] = NULL;
        return status;
    }


    return HW_UART_STATUS_OK;
}


/*
 * 通过已经初始化的 UART 发送指定长度的数据。
 *
 * 参数：
 *
 * UART_index：
 *     UART 逻辑编号。
 *
 * data：
 *     待发送数据的首地址。
 *
 * data_length：
 *     要发送的数据长度，单位为字节。
 *
 * 发送流程：
 *
 * 1. 检查参数；
 * 2. 检查 UART 是否已经初始化；
 * 3. 等待 TXE 标志；
 * 4. 写入一个字节；
 * 5. 重复发送所有字节；
 * 6. 等待 TC 标志，确保最后一个字节已经发送完成。
 */
HW_UART_Status_e HW_UART_Send(
    UART_index_e UART_index,
    const uint8_t *data,
    uint16_t data_length)
{
    USART_TypeDef *uart_instance;
    HW_UART_Status_e status;
    uint16_t data_index;


    /*
     * 检查 UART 编号、数据指针和数据长度。
     */
    if (((uint32_t)UART_index >= (uint32_t)UART_COUNT) ||
        (data == NULL) ||
        (data_length == 0U))
    {
        return HW_UART_STATUS_INVALID_ARG;
    }


    /*
     * 获取 UART 当前绑定的 USART 外设。
     */
    uart_instance = s_uart_active_instance[UART_index];


    /*
     * 如果外设为空，说明 UART 尚未初始化。
     */
    if (uart_instance == NULL)
    {
        return HW_UART_STATUS_NOT_READY;
    }


    /*
     * 循环发送所有数据。
     */
    for (data_index = 0U;
         data_index < data_length;
         data_index++)
    {
        /*
         * 等待发送数据寄存器为空。
         */
        status = HW_UART_WaitFlag(uart_instance, 0U);
        if (status != HW_UART_STATUS_OK)
        {
            return status;
        }


        /*
         * 写入一个 8 位数据到 USART 发送数据寄存器。
         */
        LL_USART_TransmitData8(
            uart_instance,
            data[data_index]);
    }


    /*
     * 等待最后一个字节完全发送完成。
     */
    return HW_UART_WaitFlag(uart_instance, 1U);
}


/*
 * 一次性接收一段不定长 UART 回复。
 *
 * 本函数每次都从用户数组 data[0] 开始保存本轮新收到的字节，
 * 最多保存 data_capacity 个字节。字节仍由 RXNE 中断写入数组，
 * 本函数只同步等待以下任一结束条件：
 *
 * 1. 收到至少一个字节后，连续 silence_timeout_ms 没有新字节；
 * 2. 整个接收过程达到 timeout_ms；
 * 3. 本轮发生 UART 接收错误。
 *
 * 返回后会关闭本轮数组的写入许可，后续字节不会继续追加到 data。
 */
HW_UART_Status_e HW_UART_Receive_IT(
    UART_index_e UART_index,
    uint8_t *data,
    uint16_t data_capacity,
    uint16_t *received_length,
    uint32_t timeout_ms,
    uint32_t silence_timeout_ms)
{
    HW_UART_RxOnce_t *rx_once;
    uint32_t interrupt_state;
    uint32_t silence_elapsed_ms = 0U;
    uint32_t last_activity_count = 0U;
    uint32_t current_activity_count;
    uint16_t final_length;
    uint8_t final_overflow;
    uint8_t final_rx_error;
    uint8_t is_complete = 0U;
    uint8_t is_timeout = 0U;


    if (received_length != NULL)
    {
        *received_length = 0U;
    }


    /* 检查实例、数组、容量以及两个毫秒时间参数。 */
    if (((uint32_t)UART_index >= (uint32_t)UART_COUNT) ||
        (data == NULL) ||
        (data_capacity == 0U) ||
        (received_length == NULL) ||
        (timeout_ms == 0U) ||
        (silence_timeout_ms == 0U))
    {
        return HW_UART_STATUS_INVALID_ARG;
    }


    /* UART 中断必须能运行，因此禁止从 ISR 或关中断临界区调用。 */
    if ((__get_IPSR() != 0U) || ((__get_PRIMASK() & 1U) != 0U))
    {
        return HW_UART_STATUS_INVALID_CONTEXT;
    }


    if (s_uart_active_instance[UART_index] == NULL)
    {
        return HW_UART_STATUS_NOT_READY;
    }


    rx_once = &s_uart_rx_once[UART_index];


    /*
     * 原子地建立本轮接收窗口。
     * active 最后置 1，确保中断看到 active 时其他成员已经有效。
     */
    interrupt_state = __get_PRIMASK();
    __disable_irq();

    if (rx_once->active != 0U)
    {
        if ((interrupt_state & 1U) == 0U)
        {
            __enable_irq();
        }
        return HW_UART_STATUS_BUSY;
    }

    rx_once->data = data;
    rx_once->capacity = data_capacity;
    rx_once->length = 0U;
    rx_once->activity_count = 0U;
    rx_once->overflow = 0U;
    rx_once->rx_error = 0U;
    rx_once->active = 1U;

    if ((interrupt_state & 1U) == 0U)
    {
        __enable_irq();
    }


    /*
     * 接收字节由 UART 中断完成；这里按 1 ms 周期判断本轮是否结束。
     * activity_count 在数组已满时仍会变化，因此不会把持续到来的
     * 超长回复误判成已经静默。
     */
    while (timeout_ms != 0U)
    {
        /* 此 LL 实现会额外加一个 tick，传 0 才等待一个 1 ms tick。 */
        LL_mDelay(0U);
        timeout_ms--;

        if (rx_once->rx_error != 0U)
        {
            break;
        }

        current_activity_count = rx_once->activity_count;

        if (current_activity_count != last_activity_count)
        {
            last_activity_count = current_activity_count;
            silence_elapsed_ms = 0U;
        }
        else if (current_activity_count != 0U)
        {
            silence_elapsed_ms++;

            if (silence_elapsed_ms >= silence_timeout_ms)
            {
                /*
                 * 锁住中断后再次确认没有新字节，避免恰好在判断边界
                 * 到来的字节被错误地排除在本轮回复之外。
                 */
                interrupt_state = __get_PRIMASK();
                __disable_irq();

                if ((rx_once->activity_count == last_activity_count) &&
                    (LL_USART_IsActiveFlag_RXNE(
                         s_uart_active_instance[UART_index]) == 0U) &&
                    (LL_USART_IsActiveFlag_ORE(
                         s_uart_active_instance[UART_index]) == 0U) &&
                    (LL_USART_IsActiveFlag_FE(
                         s_uart_active_instance[UART_index]) == 0U) &&
                    (LL_USART_IsActiveFlag_NE(
                         s_uart_active_instance[UART_index]) == 0U))
                {
                    rx_once->active = 0U;
                    is_complete = 1U;
                }
                else
                {
                    last_activity_count = rx_once->activity_count;
                    silence_elapsed_ms = 0U;
                }

                if ((interrupt_state & 1U) == 0U)
                {
                    __enable_irq();
                }

                if (is_complete != 0U)
                {
                    break;
                }
            }
        }
    }


    /* 关闭本轮写入并原子地取回最终长度及错误状态。 */
    interrupt_state = __get_PRIMASK();
    __disable_irq();

    if ((is_complete == 0U) && (rx_once->rx_error == 0U))
    {
        is_timeout = 1U;
    }

    rx_once->active = 0U;
    final_length = rx_once->length;
    final_overflow = rx_once->overflow;
    final_rx_error = rx_once->rx_error;

    rx_once->data = NULL;
    rx_once->capacity = 0U;

    if ((interrupt_state & 1U) == 0U)
    {
        __enable_irq();
    }


    *received_length = final_length;

    if (final_rx_error != 0U)
    {
        return HW_UART_STATUS_RX_ERROR;
    }

    if (final_overflow != 0U)
    {
        return HW_UART_STATUS_BUFFER_OVERFLOW;
    }

    if (is_timeout != 0U)
    {
        return HW_UART_STATUS_TIMEOUT;
    }

    return HW_UART_STATUS_OK;
}


/*
 * 从 UART 的中断环形缓冲区中读取已经接收到的数据。
 *
 * 参数：
 *
 * UART_index：
 *     UART 逻辑编号。
 *
 * data：
 *     用于保存读取数据的用户缓冲区。
 *
 * data_length：
 *     输入时表示 data 缓冲区容量；
 *     输出时表示实际读取到的数据长度。
 *
 * 返回值：
 *
 * HW_UART_STATUS_OK：
 *     成功读取到数据。
 *
 * HW_UART_STATUS_NO_DATA：
 *     当前没有可读取的数据。
 *
 * HW_UART_STATUS_BUFFER_OVERFLOW：
 *     接收缓冲区曾经发生溢出。
 *     本次读取的数据仍然会返回，但部分数据可能已经丢失。
 *
 * HW_UART_STATUS_NOT_READY：
 *     UART 尚未初始化。
 */
HW_UART_Status_e HW_UART_ReceiveRingBuffer_IT(
    UART_index_e UART_index,
    uint8_t *data,
    uint16_t *data_length)
{
    HW_UART_RxBuffer_t *rx_buffer;
    uint16_t buffer_capacity;
    uint16_t received_length = 0U;
    uint32_t interrupt_state;
    uint8_t overflow;


    /*
     * 检查 UART 编号、数据缓冲区指针和长度指针。
     */
    if (((uint32_t)UART_index >= (uint32_t)UART_COUNT) ||
        (data == NULL) ||
        (data_length == NULL))
    {
        return HW_UART_STATUS_INVALID_ARG;
    }


    /*
     * 保存用户缓冲区容量，
     * 然后将输出长度先清零。
     */
    buffer_capacity = *data_length;
    *data_length = 0U;


    /*
     * 用户缓冲区容量不能为 0。
     */
    if (buffer_capacity == 0U)
    {
        return HW_UART_STATUS_INVALID_ARG;
    }


    /*
     * 检查 UART 是否已经初始化。
     */
    if (s_uart_active_instance[UART_index] == NULL)
    {
        return HW_UART_STATUS_NOT_READY;
    }


    /*
     * 调用此接口表示本轮选择环形缓冲模式。
     * 一次性接收正在运行时，两种消费者不能同时读取同一 UART。
     */
    interrupt_state = __get_PRIMASK();
    __disable_irq();

    if (s_uart_rx_once[UART_index].active != 0U)
    {
        if ((interrupt_state & 1U) == 0U)
        {
            __enable_irq();
        }
        return HW_UART_STATUS_BUSY;
    }

    if ((interrupt_state & 1U) == 0U)
    {
        __enable_irq();
    }


    /*
     * 获取指定 UART 的接收缓冲区。
     */
    rx_buffer = &s_uart_rx_buffer[UART_index];


    /*
     * 从环形缓冲区中读取数据。
     *
     * 读取条件：
     *
     * 1. 用户缓冲区仍有剩余空间；
     * 2. 环形缓冲区中存在未读取的数据。
     */
    while ((received_length < buffer_capacity) &&
           (rx_buffer->read_index != rx_buffer->write_index))
    {
        /*
         * 通过掩码计算环形缓冲区中的实际数组下标。
         */
        data[received_length] =
            rx_buffer->data[
                rx_buffer->read_index & HW_UART_RX_BUFFER_MASK];

        rx_buffer->read_index++;
        received_length++;
    }


    /*
     * 返回本次实际读取到的数据长度。
     */
    *data_length = received_length;


    /*
     * 读取并清除接收缓冲区溢出标志。
     *
     * 读取和清除操作需要临界区保护，
     * 防止读取过程中 UART 中断同时修改 overflow 标志。
     */
    interrupt_state = __get_PRIMASK();

    __disable_irq();

    overflow = rx_buffer->overflow;
    rx_buffer->overflow = 0U;


    /*
     * 如果进入临界区之前没有关闭全局中断，
     * 则在操作完成后恢复开启全局中断。
     *
     * 如果调用本函数之前全局中断已经关闭，
     * 则保持原来的关闭状态。
     */
    if ((interrupt_state & 1U) == 0U)
    {
        __enable_irq();
    }


    /*
     * 如果接收过程中发生过缓冲区溢出，
     * 返回溢出状态。
     */
    if (overflow != 0U)
    {
        return HW_UART_STATUS_BUFFER_OVERFLOW;
    }


    /*
     * 当前没有读取到任何数据。
     */
    if (received_length == 0U)
    {
        return HW_UART_STATUS_NO_DATA;
    }


    /*
     * 成功读取数据。
     */
    return HW_UART_STATUS_OK;
}
