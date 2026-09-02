#ifndef __SERVER_JSON_PARSE_H__
#define __SERVER_JSON_PARSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* JSON 模块的执行结果，调用者应根据返回值判断本次解析是否可用。 */
typedef enum
{
    SERVER_JSON_OK = 0,
    SERVER_JSON_INVALID_ARGUMENT,
    SERVER_JSON_SYNTAX_ERROR,
    SERVER_JSON_OUTPUT_FULL,
    SERVER_JSON_UNSUPPORTED_VALUE,
} SERVER_JSON_State_e;

/*
 * 解析 UART 接收的无嵌套 JSON 对象，并把每一组“字段名:字段值”拆开保存。
 *
 * 适用数据：{"sensor":"temp","value":25,"unit":"C"}
 * 不支持数组 []、嵌套对象 {}；字符串内的转义字符保持原始字符，不做转义还原。
 *
 * 参数填写：
 * data          ：输入 JSON 的首地址，例如 UART 的 rx_data。
 * data_length   ：输入 JSON 的实际字节数，例如 UART 返回的 rx_length；不包含 '\0'。
 * keys          ：字段名二维数组的首地址，例如 (uint8_t *)key_buffer。
 * key_stride    ：keys 每一行的长度，例如 sizeof(key_buffer[0])。
 * values        ：字段值二维数组的首地址，例如 (uint8_t *)value_buffer。
 * value_stride  ：values 每一行的长度，例如 sizeof(value_buffer[0])。
 * max_pairs     ：keys/values 可保存的最大字段数量。
 * out_pair_count：输出参数，成功后返回实际解析出的字段数量。
 *
 * 成功时 keys 和 values 中每一项都以 '\0' 结尾；字符串值会去掉外层双引号。
 */
SERVER_JSON_State_e SERVER_JSON_ParseObject(const uint8_t *data,
                                            uint16_t data_length,
                                            uint8_t *keys,
                                            uint16_t key_stride,
                                            uint8_t *values,
                                            uint16_t value_stride,
                                            uint8_t max_pairs,
                                            uint8_t *out_pair_count);

/*
 * 按一个指定字符切分字节数据，适用于非 JSON 的简单协议字段。
 *
 * 参数填写：
 * data/data_length：待切分的原始数据和实际长度。
 * delimiter       ：分隔字符；按冒号拆分传 ':'，按逗号拆分传 ','。
 * fields          ：保存切分结果的二维数组首地址，例如 (uint8_t *)field_buffer。
 * field_stride    ：fields 每一行的长度，例如 sizeof(field_buffer[0])。
 * max_fields      ：fields 可保存的最大字段数量。
 * out_field_count ：输出参数，成功后返回实际切分出的字段数。
 *
 * 成功时每一个 fields 项都以 '\0' 结尾；连续分隔符之间会保留一个空字段。
 */
SERVER_JSON_State_e SERVER_JSON_SplitByDelimiter(const uint8_t *data,
                                                  uint16_t data_length,
                                                  uint8_t delimiter,
                                                  uint8_t *fields,
                                                  uint16_t field_stride,
                                                  uint8_t max_fields,
                                                  uint8_t *out_field_count);

#ifdef __cplusplus
}
#endif

#endif /* __SERVER_JSON_PARSE_H__ */
