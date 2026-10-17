#include "SERVER/ST7789/ST7789.h"
#include "HW_INIT/SPI/SPI_init.h"

/* 屏幕引脚定义，按实际接线修改。 */
#define ST7789_SPI_SCK             SCK_B3
#define ST7789_SPI_MOSI            MOSI_B5
#define ST7789_SPI_MISO            MISO_NULL

#define ST7789_PIN_CS              A6
#define ST7789_PIN_RS              A7
#define ST7789_PIN_RES             B1
#define ST7789_PIN_BLK             B2

#define ST7789_SCREEN_WIDTH        240U								//屏幕宽度
#define ST7789_SCREEN_HEIGHT       296U								//屏幕高度

#define ST7789_BUFFER_SIZE         1024U								//缓冲区大小
static uint8_t ST7789_BUFFER_Data[ST7789_BUFFER_SIZE];	//缓冲区

#define ST7789_DISPLAY_ORIENTATION 0U			//显示方向

typedef struct
{
    uint8_t command;
    const uint8_t *data;
    uint16_t data_count;
} ST7789_InitCommand_t;

static uint8_t s_st7789_initialized = 0U;

#if (ST7789_DISPLAY_ORIENTATION == 0U)
static const uint8_t s_madctl[] = {0x00U};
#elif (ST7789_DISPLAY_ORIENTATION == 1U)
static const uint8_t s_madctl[] = {0xC0U};
#elif (ST7789_DISPLAY_ORIENTATION == 2U)
static const uint8_t s_madctl[] = {0x70U};
#else
static const uint8_t s_madctl[] = {0xA0U};
#endif

//初始化命令对照值
static const uint8_t s_pixel_format[] = {0x05U};
static const uint8_t s_b2[] = {0x07U, 0x07U, 0x00U, 0x33U, 0x33U};
static const uint8_t s_b7[] = {0x05U};
static const uint8_t s_bb[] = {0x1DU};
static const uint8_t s_c0[] = {0x2CU};
static const uint8_t s_c2[] = {0x01U};
static const uint8_t s_c3[] = {0x15U};
static const uint8_t s_c6[] = {0x16U};
static const uint8_t s_d0_a7[] = {0xA7U, 0xA1U};
static const uint8_t s_d0_a4[] = {0xA4U, 0xA1U};
static const uint8_t s_d6[] = {0xA1U};
static const uint8_t s_e0[] = {
    0xF0U, 0x04U, 0x09U, 0x08U, 0x08U, 0x16U, 0x2FU,
    0x43U, 0x47U, 0x38U, 0x14U, 0x14U, 0x2BU, 0x34U
};
static const uint8_t s_e1[] = {
    0xF0U, 0x06U, 0x0BU, 0x0AU, 0x09U, 0x03U, 0x2FU,
    0x33U, 0x46U, 0x3AU, 0x16U, 0x15U, 0x2CU, 0x32U
};
static const uint8_t s_e4[] = {0x26U, 0x00U, 0x00U};

//初始化命令
static const ST7789_InitCommand_t s_init_commands[] =
{
    {0x36U, s_madctl,       sizeof(s_madctl)},       // 设置扫描方向和镜像
    {0x3AU, s_pixel_format, sizeof(s_pixel_format)}, // 设置像素格式
    {0xB2U, s_b2,           sizeof(s_b2)},           // 设置帧同步前后沿
    {0xB7U, s_b7,           sizeof(s_b7)},           // 设置栅极驱动电压
    {0xBBU, s_bb,           sizeof(s_bb)},           // 设置VCOM电压
    {0xC0U, s_c0,           sizeof(s_c0)},           // 设置液晶模块控制参数
    {0xC2U, s_c2,           sizeof(s_c2)},           // 允许命令设置VRH电压
    {0xC3U, s_c3,           sizeof(s_c3)},           // 设置VRH电压
    {0xC6U, s_c6,           sizeof(s_c6)},           // 设置正常模式帧率
    {0xD0U, s_d0_a7,        sizeof(s_d0_a7)},         // 电荷泵启动时使用高功率
    {0xD0U, s_d0_a4,        sizeof(s_d0_a4)},         // 电荷泵切换到正常功率
    {0xD6U, s_d6,           sizeof(s_d6)},            // 设置休眠时栅极输出
    {0xE0U, s_e0,           sizeof(s_e0)},            // 设置正电压伽马曲线
    {0xE1U, s_e1,           sizeof(s_e1)},            // 设置负电压伽马曲线
    {0xE4U, s_e4,           sizeof(s_e4)},            // 设置栅极数量和扫描方式
    {0x21U, NULL,           0U},                     // 开启显示反转
};

/* 发送一个命令或参数字节。 */
static ST7789_Status_e ST7789_WriteByte(uint8_t data, uint8_t is_command)
{
    ST7789_Status_e status = ST7789_STATUS_OK;

    if (HW_GPIO_SET_Pin(ST7789_PIN_CS, LOW) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }
    if (HW_GPIO_SET_Pin(ST7789_PIN_RS,
                        (is_command != 0U) ? LOW : HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
        goto release_pins;
    }

    if ((SPI_Transmit_DMA(&data, 1U) != HW_SPI_STATUS_OK) ||
        (SPI_WaitAndCheckEndOfTransfer() != HW_SPI_STATUS_OK))
    {
        status = ST7789_STATUS_SPI_ERROR;
    }

release_pins:
    if (HW_GPIO_SET_Pin(ST7789_PIN_RS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
    }
    if (HW_GPIO_SET_Pin(ST7789_PIN_CS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
    }

    return status;
}

/* 保持片选有效时发送一段命令或参数。 */
static ST7789_Status_e ST7789_WriteSelected(uint8_t *data,
                                            uint16_t length,
                                            uint8_t is_command)
{
    if (HW_GPIO_SET_Pin(ST7789_PIN_RS,
                        (is_command != 0U) ? LOW : HIGH) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }
    if ((SPI_Transmit_DMA(data, length) != HW_SPI_STATUS_OK) ||
        (SPI_WaitAndCheckEndOfTransfer() != HW_SPI_STATUS_OK))
    {
        return ST7789_STATUS_SPI_ERROR;
    }

    return ST7789_STATUS_OK;
}

/* 设置坐标窗口并填充指定颜色。 */
static ST7789_Status_e ST7789_FillArea(int32_t x_start,
                                       int32_t y_start,
                                       int32_t x_end,
                                       int32_t y_end,
                                       uint16_t color)
{
    ST7789_Status_e status = ST7789_STATUS_OK;
    uint8_t command;
    uint8_t column_data[4];
    uint8_t row_data[4];
    uint32_t chunk_limit;
    uint32_t index;
    uint16_t chunk_size;
    uint32_t width;
    uint32_t height;
    uint64_t remaining_bytes;
    uint64_t bytes_sent = 0U;

    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }
    if ((x_start > x_end) || (y_start > y_end))
    {
        return ST7789_STATUS_INVALID_ARG;
    }
    if ((x_end < 0) || (y_end < 0) ||
        (x_start >= (int32_t)ST7789_SCREEN_WIDTH) ||
        (y_start >= (int32_t)ST7789_SCREEN_HEIGHT))
    {
        return ST7789_STATUS_OK;
    }

    if (x_start < 0)
    {
        x_start = 0;
    }
    if (y_start < 0)
    {
        y_start = 0;
    }
    if (x_end >= (int32_t)ST7789_SCREEN_WIDTH)
    {
        x_end = (int32_t)ST7789_SCREEN_WIDTH - 1;
    }
    if (y_end >= (int32_t)ST7789_SCREEN_HEIGHT)
    {
        y_end = (int32_t)ST7789_SCREEN_HEIGHT - 1;
    }

    width = (uint32_t)(x_end - x_start + 1);
    height = (uint32_t)(y_end - y_start + 1);
    remaining_bytes = (uint64_t)width * (uint64_t)height * 2U;
    chunk_limit = ST7789_BUFFER_SIZE;
    if ((chunk_limit == 0U) || (remaining_bytes == 0U))
    {
        return ST7789_STATUS_INVALID_ARG;
    }
    if (chunk_limit > 0xFFFFUL)
    {
        chunk_limit = 0xFFFFUL;
    }

    column_data[0] = (uint8_t)((uint32_t)x_start >> 8);
    column_data[1] = (uint8_t)x_start;
    column_data[2] = (uint8_t)((uint32_t)x_end >> 8);
    column_data[3] = (uint8_t)x_end;
    row_data[0] = (uint8_t)((uint32_t)y_start >> 8);
    row_data[1] = (uint8_t)y_start;
    row_data[2] = (uint8_t)((uint32_t)y_end >> 8);
    row_data[3] = (uint8_t)y_end;

    if (HW_GPIO_SET_Pin(ST7789_PIN_CS, LOW) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }

    command = 0x2AU;
    status = ST7789_WriteSelected(&command, 1U, 1U);
    if (status != ST7789_STATUS_OK)
    {
        goto release_area_pins;
    }
    status = ST7789_WriteSelected(column_data, sizeof(column_data), 0U);
    if (status != ST7789_STATUS_OK)
    {
        goto release_area_pins;
    }

    command = 0x2BU;
    status = ST7789_WriteSelected(&command, 1U, 1U);
    if (status != ST7789_STATUS_OK)
    {
        goto release_area_pins;
    }
    status = ST7789_WriteSelected(row_data, sizeof(row_data), 0U);
    if (status != ST7789_STATUS_OK)
    {
        goto release_area_pins;
    }

    command = 0x2CU;
    status = ST7789_WriteSelected(&command, 1U, 1U);
    if (status != ST7789_STATUS_OK)
    {
        goto release_area_pins;
    }

    while (remaining_bytes > 0U)
    {
        chunk_size = (remaining_bytes > chunk_limit) ?
                     (uint16_t)chunk_limit : (uint16_t)remaining_bytes;

        for (index = 0U; index < chunk_size; index++)
        {
            ST7789_BUFFER_Data[index] =
                (((bytes_sent + index) & 1U) == 0U) ?
                (uint8_t)(color >> 8) : (uint8_t)color;
        }

        if (HW_GPIO_SET_Pin(ST7789_PIN_RS, HIGH) != HW_GPIO_STATUS_OK)
        {
            status = ST7789_STATUS_GPIO_ERROR;
            goto release_area_pins;
        }
        if ((SPI_Transmit_DMA(ST7789_BUFFER_Data, chunk_size) != HW_SPI_STATUS_OK) ||
            (SPI_WaitAndCheckEndOfTransfer() != HW_SPI_STATUS_OK))
        {
            status = ST7789_STATUS_SPI_ERROR;
            goto release_area_pins;
        }

        remaining_bytes -= chunk_size;
        bytes_sent += chunk_size;
    }

release_area_pins:
    if (HW_GPIO_SET_Pin(ST7789_PIN_RS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
    }
    if (HW_GPIO_SET_Pin(ST7789_PIN_CS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
    }

    return status;
}

/* 设置图片写入窗口。 */
static ST7789_Status_e ST7789_SetWindowSelected(uint16_t x_start,
                                                uint16_t y_start,
                                                uint16_t x_end,
                                                uint16_t y_end)
{
    ST7789_Status_e status;
    uint8_t command;
    uint8_t address_data[4];

    command = 0x2AU;
    status = ST7789_WriteSelected(&command, 1U, 1U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }
    address_data[0] = (uint8_t)(x_start >> 8);
    address_data[1] = (uint8_t)x_start;
    address_data[2] = (uint8_t)(x_end >> 8);
    address_data[3] = (uint8_t)x_end;
    status = ST7789_WriteSelected(address_data, sizeof(address_data), 0U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }

    command = 0x2BU;
    status = ST7789_WriteSelected(&command, 1U, 1U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }
    address_data[0] = (uint8_t)(y_start >> 8);
    address_data[1] = (uint8_t)y_start;
    address_data[2] = (uint8_t)(y_end >> 8);
    address_data[3] = (uint8_t)y_end;
    status = ST7789_WriteSelected(address_data, sizeof(address_data), 0U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }

    command = 0x2CU;
    return ST7789_WriteSelected(&command, 1U, 1U);
}

/* 发送裁剪后的RGB565图片数据。 */
static ST7789_Status_e ST7789_WriteImageArea(int32_t x_start,
                                             int32_t y_start,
                                             int32_t x_end,
                                             int32_t y_end,
                                             uint16_t source_width,
                                             uint32_t source_x_start,
                                             uint32_t source_y_start,
                                             const uint16_t *image)
{
    ST7789_Status_e status = ST7789_STATUS_OK;
    uint32_t visible_width;
    uint32_t chunk_limit;
    uint32_t index;
    uint16_t chunk_size;
    uint32_t chunk_pixels;
    uint64_t source_x;
    uint64_t source_y;
    uint64_t visible_pixels;
    uint64_t pixels_sent = 0U;
    uint64_t remaining_pixels;

    visible_width = (uint32_t)(x_end - x_start + 1);
    visible_pixels = (uint64_t)visible_width *
                     (uint64_t)(y_end - y_start + 1);

    chunk_limit = ST7789_BUFFER_SIZE;
    if ((chunk_limit < 2U) || (visible_pixels == 0U))
    {
        return ST7789_STATUS_INVALID_ARG;
    }
    if (chunk_limit > 0xFFFEUL)
    {
        chunk_limit = 0xFFFEUL;
    }
    chunk_limit &= ~1UL;

    if (HW_GPIO_SET_Pin(ST7789_PIN_CS, LOW) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }

    status = ST7789_SetWindowSelected((uint16_t)x_start,
                                      (uint16_t)y_start,
                                      (uint16_t)x_end,
                                      (uint16_t)y_end);
    if (status != ST7789_STATUS_OK)
    {
        goto release_image_pins;
    }
    if (HW_GPIO_SET_Pin(ST7789_PIN_RS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
        goto release_image_pins;
    }

    remaining_pixels = visible_pixels;
    while (remaining_pixels > 0U)
    {
        chunk_pixels = (remaining_pixels > (chunk_limit / 2U)) ?
                       (chunk_limit / 2U) : (uint32_t)remaining_pixels;
        chunk_size = (uint16_t)(chunk_pixels * 2U);

        for (index = 0U; index < chunk_pixels; index++)
        {
            uint64_t image_index;
            uint16_t pixel;

            source_x = (pixels_sent + index) % visible_width;
            source_y = (pixels_sent + index) / visible_width;
            image_index = ((uint64_t)source_y_start + source_y) *
                          source_width + source_x_start + source_x;
            pixel = image[image_index];
            ST7789_BUFFER_Data[index * 2U] = (uint8_t)(pixel >> 8);
            ST7789_BUFFER_Data[(index * 2U) + 1U] = (uint8_t)pixel;
        }

        if ((SPI_Transmit_DMA(ST7789_BUFFER_Data, chunk_size) != HW_SPI_STATUS_OK) ||
            (SPI_WaitAndCheckEndOfTransfer() != HW_SPI_STATUS_OK))
        {
            status = ST7789_STATUS_SPI_ERROR;
            goto release_image_pins;
        }

        pixels_sent += chunk_pixels;
        remaining_pixels -= chunk_pixels;
    }

release_image_pins:
    if (HW_GPIO_SET_Pin(ST7789_PIN_RS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
    }
    if (HW_GPIO_SET_Pin(ST7789_PIN_CS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
    }

    return status;
}

/* 发送一条命令。 */
static ST7789_Status_e ST7789_WriteCommand(uint8_t command)
{
    return ST7789_WriteByte(command, 1U);
}

/* 发送一个参数字节。 */
static ST7789_Status_e ST7789_WriteData(uint8_t data)
{
    return ST7789_WriteByte(data, 0U);
}

/* 初始化屏幕控制引脚。 */
static ST7789_Status_e ST7789_InitGPIO(void)
{
    if ((HW_GPIO_OUT_init(ST7789_PIN_CS, NO) != HW_GPIO_STATUS_OK) ||
        (HW_GPIO_OUT_init(ST7789_PIN_RS, NO) != HW_GPIO_STATUS_OK) ||
        (HW_GPIO_OUT_init(ST7789_PIN_RES, NO) != HW_GPIO_STATUS_OK) ||
        (HW_GPIO_OUT_init(ST7789_PIN_BLK, NO) != HW_GPIO_STATUS_OK))
    {
        return ST7789_STATUS_GPIO_ERROR;
    }

    if ((HW_GPIO_SET_Pin(ST7789_PIN_CS, HIGH) != HW_GPIO_STATUS_OK) ||
        (HW_GPIO_SET_Pin(ST7789_PIN_RS, HIGH) != HW_GPIO_STATUS_OK) ||
        (HW_GPIO_SET_Pin(ST7789_PIN_RES, HIGH) != HW_GPIO_STATUS_OK) ||
        (HW_GPIO_SET_Pin(ST7789_PIN_BLK, LOW) != HW_GPIO_STATUS_OK))
    {
        return ST7789_STATUS_GPIO_ERROR;
    }

    return ST7789_STATUS_OK;
}

/* 初始化屏幕、SPI 和背光（写的很全）*/
ST7789_Status_e ST7789_Init(void)
{
    ST7789_Status_e status;
    uint32_t index;
    uint16_t data_index;

    s_st7789_initialized = 0U;

    status = ST7789_InitGPIO();
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }

    if (HW_SPI_DMA_init(SPI_BUS1,
                        ST7789_SPI_SCK,
                        ST7789_SPI_MOSI,
                        ST7789_SPI_MISO) != HW_SPI_STATUS_OK)
    {
        return ST7789_STATUS_SPI_ERROR;
    }

    /* 屏幕复位。 */
    if (HW_GPIO_SET_Pin(ST7789_PIN_RES, HIGH) != HW_GPIO_STATUS_OK){
        return ST7789_STATUS_GPIO_ERROR;
    }
    LL_mDelay(30U);
    if (HW_GPIO_SET_Pin(ST7789_PIN_RES, LOW) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }
    LL_mDelay(30U);
    if (HW_GPIO_SET_Pin(ST7789_PIN_RES, HIGH) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }
    LL_mDelay(120U);

    if (HW_GPIO_SET_Pin(ST7789_PIN_BLK, HIGH) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }

    status = ST7789_WriteCommand(0x11U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }
    LL_mDelay(120U);

    for (index = 0U;
         index < (sizeof(s_init_commands) / sizeof(s_init_commands[0]));
         index++)
    {
        status = ST7789_WriteCommand(s_init_commands[index].command);
        if (status != ST7789_STATUS_OK)
        {
            return status;
        }

        for (data_index = 0U;
             data_index < s_init_commands[index].data_count;
             data_index++)
        {
            status = ST7789_WriteData(s_init_commands[index].data[data_index]);
            if (status != ST7789_STATUS_OK)
            {
                return status;
            }
        }
    }

    status = ST7789_WriteCommand(0x11U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }
    LL_mDelay(120U);

    status = ST7789_WriteCommand(0x29U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }

    s_st7789_initialized = 1U;
    return ST7789_STATUS_OK;
}

/* 发送一条命令。 */
ST7789_Status_e ST7789_COMMAND(uint8_t command)
{
    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }

    return ST7789_WriteCommand(command);
}

/* 发送一个参数字节。 */
ST7789_Status_e ST7789_DATA(uint8_t data)
{
    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }

    return ST7789_WriteData(data);
}

/* 连续发送数据，整个过程保持片选有效。 */
ST7789_Status_e ST7789_DATA_BUFFER(const uint8_t *data, uint32_t length)
{
    ST7789_Status_e status = ST7789_STATUS_OK;
    uint32_t remaining;
    uint32_t index;
    uint16_t chunk_size;

    if ((data == NULL) || (length == 0U))
    {
        return ST7789_STATUS_INVALID_ARG;
    }
    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }

    if (HW_GPIO_SET_Pin(ST7789_PIN_CS, LOW) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }
    if (HW_GPIO_SET_Pin(ST7789_PIN_RS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
        goto release_pins;
    }

    remaining = length;
    while (remaining > 0U)
    {
        chunk_size = (remaining > ST7789_BUFFER_SIZE) ?
                     ST7789_BUFFER_SIZE : (uint16_t)remaining;

        for (index = 0U; index < chunk_size; index++)
        {
            ST7789_BUFFER_Data[index] = data[index];
        }

        if ((SPI_Transmit_DMA(ST7789_BUFFER_Data, chunk_size) != HW_SPI_STATUS_OK) ||
            (SPI_WaitAndCheckEndOfTransfer() != HW_SPI_STATUS_OK))
        {
            status = ST7789_STATUS_SPI_ERROR;
            break;
        }

        data += chunk_size;
        remaining -= chunk_size;
    }

release_pins:
    if (HW_GPIO_SET_Pin(ST7789_PIN_RS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
    }
    if (HW_GPIO_SET_Pin(ST7789_PIN_CS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
    }

    return status;
}

/* 按RGB565颜色填充整屏。 */
ST7789_Status_e ST7789_FillColor(uint16_t color)
{
    ST7789_Status_e status = ST7789_STATUS_OK;
    uint8_t command;
    uint8_t column_data[4];
    uint8_t row_data[4];
    uint32_t chunk_limit;
    uint32_t index;
    uint16_t chunk_size;
    uint64_t remaining_bytes;
    uint64_t bytes_sent = 0U;

    if ((ST7789_SCREEN_WIDTH == 0U) || (ST7789_SCREEN_HEIGHT == 0U) ||
        (ST7789_SCREEN_WIDTH > 0x10000UL) ||
        (ST7789_SCREEN_HEIGHT > 0x10000UL) ||
        (ST7789_BUFFER_SIZE == 0U))
    {
        return ST7789_STATUS_INVALID_ARG;
    }
    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }

    column_data[0] = 0U;
    column_data[1] = 0U;
    column_data[2] = (uint8_t)((ST7789_SCREEN_WIDTH - 1U) >> 8);
    column_data[3] = (uint8_t)(ST7789_SCREEN_WIDTH - 1U);
    row_data[0] = 0U;
    row_data[1] = 0U;
    row_data[2] = (uint8_t)((ST7789_SCREEN_HEIGHT - 1U) >> 8);
    row_data[3] = (uint8_t)(ST7789_SCREEN_HEIGHT - 1U);

    chunk_limit = ST7789_BUFFER_SIZE;
    if (chunk_limit > 0xFFFFUL)
    {
        chunk_limit = 0xFFFFUL;
    }
    remaining_bytes = (uint64_t)ST7789_SCREEN_WIDTH *
                      (uint64_t)ST7789_SCREEN_HEIGHT * 2U;

    if (HW_GPIO_SET_Pin(ST7789_PIN_CS, LOW) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }

    command = 0x2AU;
    status = ST7789_WriteSelected(&command, 1U, 1U);
    if (status != ST7789_STATUS_OK)
    {
        goto release_fill_pins;
    }
    status = ST7789_WriteSelected(column_data, sizeof(column_data), 0U);
    if (status != ST7789_STATUS_OK)
    {
        goto release_fill_pins;
    }

    command = 0x2BU;
    status = ST7789_WriteSelected(&command, 1U, 1U);
    if (status != ST7789_STATUS_OK)
    {
        goto release_fill_pins;
    }
    status = ST7789_WriteSelected(row_data, sizeof(row_data), 0U);
    if (status != ST7789_STATUS_OK)
    {
        goto release_fill_pins;
    }

    command = 0x2CU;
    status = ST7789_WriteSelected(&command, 1U, 1U);
    if (status != ST7789_STATUS_OK)
    {
        goto release_fill_pins;
    }

    while (remaining_bytes > 0U)
    {
        chunk_size = (remaining_bytes > chunk_limit) ?
                     (uint16_t)chunk_limit : (uint16_t)remaining_bytes;

        for (index = 0U; index < chunk_size; index++)
        {
            ST7789_BUFFER_Data[index] =
                (((bytes_sent + index) & 1U) == 0U) ?
                (uint8_t)(color >> 8) : (uint8_t)color;
        }

        if (HW_GPIO_SET_Pin(ST7789_PIN_RS, HIGH) != HW_GPIO_STATUS_OK)
        {
            status = ST7789_STATUS_GPIO_ERROR;
            goto release_fill_pins;
        }
        if ((SPI_Transmit_DMA(ST7789_BUFFER_Data, chunk_size) != HW_SPI_STATUS_OK) ||
            (SPI_WaitAndCheckEndOfTransfer() != HW_SPI_STATUS_OK))
        {
            status = ST7789_STATUS_SPI_ERROR;
            goto release_fill_pins;
        }

        remaining_bytes -= chunk_size;
        bytes_sent += chunk_size;
    }

release_fill_pins:
    if (HW_GPIO_SET_Pin(ST7789_PIN_RS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
    }
    if (HW_GPIO_SET_Pin(ST7789_PIN_CS, HIGH) != HW_GPIO_STATUS_OK)
    {
        status = ST7789_STATUS_GPIO_ERROR;
    }

    return status;
}

/* 在指定坐标绘制一个像素。 */
ST7789_Status_e ST7789_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
    return ST7789_FillArea(x, y, x, y, color);
}

/* 使用Bresenham算法绘制直线。 */
ST7789_Status_e ST7789_DrawLine(int16_t x_start,
                                int16_t y_start,
                                int16_t x_end,
                                int16_t y_end,
                                uint16_t color)
{
    ST7789_Status_e status;
    int32_t x = x_start;
    int32_t y = y_start;
    int32_t target_x = x_end;
    int32_t target_y = y_end;
    int32_t delta_x = (target_x >= x) ? (target_x - x) : (x - target_x);
    int32_t step_x = (x < target_x) ? 1 : -1;
    int32_t delta_y = -((target_y >= y) ? (target_y - y) : (y - target_y));
    int32_t step_y = (y < target_y) ? 1 : -1;
    int32_t error = delta_x + delta_y;
    int32_t twice_error;

    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }

    if (x_start == target_x)
    {
        int32_t top = (y_start < target_y) ? y_start : target_y;
        int32_t bottom = (y_start > target_y) ? y_start : target_y;
        return ST7789_FillArea(x_start, top, target_x, bottom, color);
    }
    if (y_start == target_y)
    {
        int32_t left = (x_start < target_x) ? x_start : target_x;
        int32_t right = (x_start > target_x) ? x_start : target_x;
        return ST7789_FillArea(left, y_start, right, target_y, color);
    }

    while (1)
    {
        if ((x >= 0) && (x < (int32_t)ST7789_SCREEN_WIDTH) &&
            (y >= 0) && (y < (int32_t)ST7789_SCREEN_HEIGHT))
        {
            status = ST7789_FillArea(x, y, x, y, color);
            if (status != ST7789_STATUS_OK)
            {
                return status;
            }
        }

        if ((x == target_x) && (y == target_y))
        {
            break;
        }

        twice_error = 2 * error;
        if (twice_error >= delta_y)
        {
            error += delta_y;
            x += step_x;
        }
        if (twice_error <= delta_x)
        {
            error += delta_x;
            y += step_y;
        }
    }

    return ST7789_STATUS_OK;
}

/* 绘制空心矩形。 */
ST7789_Status_e ST7789_DrawRect(int16_t x,
                                int16_t y,
                                uint16_t width,
                                uint16_t height,
                                uint16_t color)
{
    ST7789_Status_e status;
    int32_t right;
    int32_t bottom;

    if ((width == 0U) || (height == 0U))
    {
        return ST7789_STATUS_INVALID_ARG;
    }

    right = (int32_t)x + width - 1;
    bottom = (int32_t)y + height - 1;

    status = ST7789_FillArea(x, y, right, y, color);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }
    if (height > 1U)
    {
        status = ST7789_FillArea(x, bottom, right, bottom, color);
        if (status != ST7789_STATUS_OK)
        {
            return status;
        }
    }
    if (height > 2U)
    {
        status = ST7789_FillArea(x, (int32_t)y + 1, x, bottom - 1, color);
        if (status != ST7789_STATUS_OK)
        {
            return status;
        }
        if (width > 1U)
        {
            status = ST7789_FillArea(right, (int32_t)y + 1,
                                     right, bottom - 1, color);
            if (status != ST7789_STATUS_OK)
            {
                return status;
            }
        }
    }

    return ST7789_STATUS_OK;
}

/* 绘制实心矩形。 */
ST7789_Status_e ST7789_FillRect(int16_t x,
                                int16_t y,
                                uint16_t width,
                                uint16_t height,
                                uint16_t color)
{
    if ((width == 0U) || (height == 0U))
    {
        return ST7789_STATUS_INVALID_ARG;
    }

    return ST7789_FillArea(x, y,
                           (int32_t)x + width - 1,
                           (int32_t)y + height - 1,
                           color);
}

/* 使用中点算法绘制空心圆。 */
ST7789_Status_e ST7789_DrawCircle(int16_t center_x,
                                  int16_t center_y,
                                  uint16_t radius,
                                  uint16_t color)
{
    ST7789_Status_e status;
    int32_t x = radius;
    int32_t y = 0;
    int32_t error = 1 - (int32_t)radius;
    int32_t cx = center_x;
    int32_t cy = center_y;

    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }
    if (radius == 0U)
    {
        return ST7789_DrawPixel(center_x, center_y, color);
    }

    while (x >= y)
    {
        status = ST7789_FillArea(cx + x, cy + y, cx + x, cy + y, color);
        if (status != ST7789_STATUS_OK) return status;
        status = ST7789_FillArea(cx + y, cy + x, cx + y, cy + x, color);
        if (status != ST7789_STATUS_OK) return status;
        status = ST7789_FillArea(cx - y, cy + x, cx - y, cy + x, color);
        if (status != ST7789_STATUS_OK) return status;
        status = ST7789_FillArea(cx - x, cy + y, cx - x, cy + y, color);
        if (status != ST7789_STATUS_OK) return status;
        status = ST7789_FillArea(cx - x, cy - y, cx - x, cy - y, color);
        if (status != ST7789_STATUS_OK) return status;
        status = ST7789_FillArea(cx - y, cy - x, cx - y, cy - x, color);
        if (status != ST7789_STATUS_OK) return status;
        status = ST7789_FillArea(cx + y, cy - x, cx + y, cy - x, color);
        if (status != ST7789_STATUS_OK) return status;
        status = ST7789_FillArea(cx + x, cy - y, cx + x, cy - y, color);
        if (status != ST7789_STATUS_OK) return status;

        y++;
        if (error < 0)
        {
            error += (2 * y) + 1;
        }
        else
        {
            x--;
            error += (2 * (y - x)) + 1;
        }
    }

    return ST7789_STATUS_OK;
}

/* 使用中点算法绘制实心圆。 */
ST7789_Status_e ST7789_FillCircle(int16_t center_x,
                                  int16_t center_y,
                                  uint16_t radius,
                                  uint16_t color)
{
    ST7789_Status_e status;
    int32_t x = radius;
    int32_t y = 0;
    int32_t error = 1 - (int32_t)radius;
    int32_t cx = center_x;
    int32_t cy = center_y;

    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }

    while (x >= y)
    {
        status = ST7789_FillArea(cx - x, cy + y, cx + x, cy + y, color);
        if (status != ST7789_STATUS_OK) return status;
        if (y != 0)
        {
            status = ST7789_FillArea(cx - x, cy - y, cx + x, cy - y, color);
            if (status != ST7789_STATUS_OK) return status;
        }
        if (x != y)
        {
            status = ST7789_FillArea(cx - y, cy + x, cx + y, cy + x, color);
            if (status != ST7789_STATUS_OK) return status;
            if (x != 0)
            {
                status = ST7789_FillArea(cx - y, cy - x, cx + y, cy - x, color);
                if (status != ST7789_STATUS_OK) return status;
            }
        }

        y++;
        if (error < 0)
        {
            error += (2 * y) + 1;
        }
        else
        {
            x--;
            error += (2 * (y - x)) + 1;
        }
    }

    return ST7789_STATUS_OK;
}

/* 显示RGB565图片，超出屏幕的部分自动裁剪。 */
ST7789_Status_e ST7789_DrawImage(int16_t x,
                                 int16_t y,
                                 uint16_t width,
                                 uint16_t height,
                                 const uint16_t *image)
{
    int32_t x_start = x;
    int32_t y_start = y;
    int32_t x_end;
    int32_t y_end;
    uint32_t source_x_start;
    uint32_t source_y_start;

    if ((image == NULL) || (width == 0U) || (height == 0U))
    {
        return ST7789_STATUS_INVALID_ARG;
    }
    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }

    x_end = x_start + (int32_t)width - 1;
    y_end = y_start + (int32_t)height - 1;
    if ((x_end < 0) || (y_end < 0) ||
        (x_start >= (int32_t)ST7789_SCREEN_WIDTH) ||
        (y_start >= (int32_t)ST7789_SCREEN_HEIGHT))
    {
        return ST7789_STATUS_OK;
    }

    source_x_start = (x_start < 0) ? (uint32_t)(-x_start) : 0U;
    source_y_start = (y_start < 0) ? (uint32_t)(-y_start) : 0U;
    if (x_start < 0)
    {
        x_start = 0;
    }
    if (y_start < 0)
    {
        y_start = 0;
    }
    if (x_end >= (int32_t)ST7789_SCREEN_WIDTH)
    {
        x_end = (int32_t)ST7789_SCREEN_WIDTH - 1;
    }
    if (y_end >= (int32_t)ST7789_SCREEN_HEIGHT)
    {
        y_end = (int32_t)ST7789_SCREEN_HEIGHT - 1;
    }

    return ST7789_WriteImageArea(x_start, y_start, x_end, y_end,
                                 width, source_x_start, source_y_start, image);
}

/* 初始化并立即显示动画首帧。 */
ST7789_Status_e ST7789_AnimationInit(ST7789_Animation_t *animation,
                                     const uint16_t * const frames[],
                                     uint16_t frame_count,
                                     int16_t x,
                                     int16_t y,
                                     uint16_t width,
                                     uint16_t height,
                                     uint32_t frame_interval_ms,
                                     uint32_t now_ms)
{
    ST7789_Status_e status;
    uint16_t index;

    if ((animation == NULL) || (frames == NULL) || (frame_count == 0U) ||
        (width == 0U) || (height == 0U) || (frame_interval_ms == 0U))
    {
        return ST7789_STATUS_INVALID_ARG;
    }
    for (index = 0U; index < frame_count; index++)
    {
        if (frames[index] == NULL)
        {
            return ST7789_STATUS_INVALID_ARG;
        }
    }

    animation->frames = frames;
    animation->frame_count = frame_count;
    animation->current_frame = 0U;
    animation->x = x;
    animation->y = y;
    animation->width = width;
    animation->height = height;
    animation->frame_interval_ms = frame_interval_ms;
    animation->last_update_ms = now_ms;
    animation->running = 1U;

    status = ST7789_DrawImage(x, y, width, height, frames[0]);
    if (status != ST7789_STATUS_OK)
    {
        animation->running = 0U;
    }

    return status;
}

/* 到达帧间隔后切换到动画帧。 */
ST7789_Status_e ST7789_AnimationUpdate(ST7789_Animation_t *animation,
                                       uint32_t now_ms)
{
    uint32_t elapsed_ms;
    uint32_t frames_elapsed;

    if ((animation == NULL) || (animation->running == 0U) ||
        (animation->frames == NULL) || (animation->frame_count == 0U) ||
        (animation->frame_interval_ms == 0U))
    {
        return ST7789_STATUS_INVALID_ARG;
    }

    elapsed_ms = now_ms - animation->last_update_ms;
    frames_elapsed = elapsed_ms / animation->frame_interval_ms;
    if (frames_elapsed == 0U)
    {
        return ST7789_STATUS_OK;
    }

    animation->current_frame = (uint16_t)
        ((animation->current_frame + (frames_elapsed % animation->frame_count)) %
         animation->frame_count);
    animation->last_update_ms += frames_elapsed * animation->frame_interval_ms;

    return ST7789_DrawImage(animation->x,
                            animation->y,
                            animation->width,
                            animation->height,
                            animation->frames[animation->current_frame]);
}

/* 停止动画。 */
ST7789_Status_e ST7789_AnimationStop(ST7789_Animation_t *animation)
{
    if (animation == NULL)
    {
        return ST7789_STATUS_INVALID_ARG;
    }

    animation->running = 0U;
    return ST7789_STATUS_OK;
}

/* 关闭显示并进入休眠。 */
ST7789_Status_e ST7789_Sleep(void)
{
    ST7789_Status_e status;

    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }

    status = ST7789_WriteCommand(0x28U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }

    status = ST7789_WriteCommand(0x10U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }
    LL_mDelay(5U);

    if (HW_GPIO_SET_Pin(ST7789_PIN_BLK, LOW) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }

    return ST7789_STATUS_OK;
}

/* 退出休眠并重新打开显示。 */
ST7789_Status_e ST7789_WakeUp(void)
{
    ST7789_Status_e status;

    if (s_st7789_initialized == 0U)
    {
        return ST7789_STATUS_NOT_READY;
    }

    status = ST7789_WriteCommand(0x11U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }
    LL_mDelay(120U);

    status = ST7789_WriteCommand(0x29U);
    if (status != ST7789_STATUS_OK)
    {
        return status;
    }

    if (HW_GPIO_SET_Pin(ST7789_PIN_BLK, HIGH) != HW_GPIO_STATUS_OK)
    {
        return ST7789_STATUS_GPIO_ERROR;
    }

    return ST7789_STATUS_OK;
}
