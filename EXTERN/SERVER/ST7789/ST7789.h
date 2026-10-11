#ifndef __SERVER_ST7789_H__
#define __SERVER_ST7789_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 常用RGB565颜色。 */
#define ST7789_COLOR_BLACK       0x0000U
#define ST7789_COLOR_WHITE       0xFFFFU
#define ST7789_COLOR_RED         0xF800U
#define ST7789_COLOR_GREEN       0x07E0U
#define ST7789_COLOR_BLUE        0x001FU
#define ST7789_COLOR_YELLOW      0xFFE0U
#define ST7789_COLOR_CYAN        0x07FFU
#define ST7789_COLOR_MAGENTA     0xF81FU
#define ST7789_COLOR_ORANGE      0xFD20U
#define ST7789_COLOR_PURPLE      0x801FU
#define ST7789_COLOR_GRAY        0x8410U
#define ST7789_COLOR_LIGHTGRAY   0xC618U
#define ST7789_COLOR_DARKGRAY    0x4208U

typedef enum
{
    ST7789_STATUS_OK = 0,
    ST7789_STATUS_INVALID_ARG,
    ST7789_STATUS_NOT_READY,
    ST7789_STATUS_GPIO_ERROR,
    ST7789_STATUS_SPI_ERROR,
} ST7789_Status_e;

typedef struct
{
    const uint16_t * const *frames;
    uint16_t frame_count;
    uint16_t current_frame;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    uint32_t frame_interval_ms;
    uint32_t last_update_ms;
    uint8_t running;
} ST7789_Animation_t;

/* 初始化屏幕、SPI 和背光。 */
ST7789_Status_e ST7789_Init(void);

/* 发送一条命令。 */
ST7789_Status_e ST7789_COMMAND(uint8_t command);
/* 发送一个参数字节。 */
ST7789_Status_e ST7789_DATA(uint8_t data);
/* 连续发送一段参数或像素数据。 */
ST7789_Status_e ST7789_DATA_BUFFER(const uint8_t *data, uint32_t length);
/* 关闭显示并进入休眠。 */
ST7789_Status_e ST7789_Sleep(void);
/* 退出休眠并重新打开显示。 */
ST7789_Status_e ST7789_WakeUp(void);


/*绘制---------------------------------------------------------------------------------------------*/
/* 按RGB565颜色填充整屏。 */
ST7789_Status_e ST7789_FillColor(uint16_t color);
/* 绘制一个像素*/
ST7789_Status_e ST7789_DrawPixel(int16_t x, int16_t y, uint16_t color);
/* 绘制任意直线*/
ST7789_Status_e ST7789_DrawLine(int16_t x_start,int16_t y_start,int16_t x_end,int16_t y_end,uint16_t color);
/* 绘制空心矩形*/
ST7789_Status_e ST7789_DrawRect(int16_t x,int16_t y,uint16_t width,uint16_t height,uint16_t color);
/* 绘制实心矩形*/
ST7789_Status_e ST7789_FillRect(int16_t x,int16_t y,uint16_t width,uint16_t height,uint16_t color);
/* 绘制空心圆*/
ST7789_Status_e ST7789_DrawCircle(int16_t center_x,int16_t center_y,uint16_t radius,uint16_t color);
/* 绘制实心圆*/
ST7789_Status_e ST7789_FillCircle(int16_t center_x,int16_t center_y,uint16_t radius,uint16_t color);

/* 显示按行排列的RGB565图片，图片数据需保持有效。 */
ST7789_Status_e ST7789_DrawImage(int16_t x,int16_t y,uint16_t width,uint16_t height,const uint16_t *image);


/*动画相关*/
/* 初始化循环动画，帧表和图片数据需保持有效。 */
ST7789_Status_e ST7789_AnimationInit(ST7789_Animation_t *animation,const uint16_t * const frames[],uint16_t frame_count,int16_t x,int16_t y,uint16_t width,uint16_t height,uint32_t frame_interval_ms,uint32_t now_ms);
/* 按毫秒计数推进动画。 */
ST7789_Status_e ST7789_AnimationUpdate(ST7789_Animation_t *animation,uint32_t now_ms);
/* 停止动画。 */
ST7789_Status_e ST7789_AnimationStop(ST7789_Animation_t *animation);


#ifdef __cplusplus
}
#endif

#endif /* __SERVER_ST7789_H__ */
