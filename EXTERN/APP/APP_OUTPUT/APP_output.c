#include "./APP/APP_OUTPUT/APP_output.h"

/* 初始化输出业务所需的 GPIO。 */
APP_OUTPUT_State_e APP_Output_init(void)
{
    if ((HW_GPIO_OUT_init(LED1, DOWN) != HW_GPIO_STATUS_OK) ||
        (HW_GPIO_OUT_init(LED2, DOWN) != HW_GPIO_STATUS_OK))
    {
        return APP_OUTPUT_FAIL;
    }

    return APP_OUTPUT_OK;
}

APP_OUTPUT_State_e APP_Output_LED1(uint8_t state)
{
    if (state > 1U) return APP_OUTPUT_FAIL;
    return (HW_GPIO_SET_Pin(LED1, state ? HIGH : LOW) == HW_GPIO_STATUS_OK) ? APP_OUTPUT_OK : APP_OUTPUT_FAIL;
}

APP_OUTPUT_State_e APP_Output_LED2(uint8_t state)
{
    if (state > 1U) return APP_OUTPUT_FAIL;
    return (HW_GPIO_SET_Pin(LED2, state ? HIGH : LOW) == HW_GPIO_STATUS_OK) ? APP_OUTPUT_OK : APP_OUTPUT_FAIL;
}
