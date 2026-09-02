#include "./SERVER/JSON/JSON_Parse.h"

/* 解析 UART 接收的扁平 JSON 对象，并输出每一组字段名和字段值。 */
SERVER_JSON_State_e SERVER_JSON_ParseObject(const uint8_t *data,
                                            uint16_t data_length,
                                            uint8_t *keys,
                                            uint16_t key_stride,
                                            uint8_t *values,
                                            uint16_t value_stride,
                                            uint8_t max_pairs,
                                            uint8_t *out_pair_count)
{
    uint16_t pos = 0U;
    uint16_t write_pos;
    uint8_t pair_count = 0U;
    uint8_t quoted_value;
    uint8_t escaped;
    uint8_t close_allowed = 1U;

    if ((data == NULL) || (data_length == 0U) || (keys == NULL) ||
        (values == NULL) || (key_stride < 2U) || (value_stride < 2U) ||
        (max_pairs == 0U) || (out_pair_count == NULL))
    {
        return SERVER_JSON_INVALID_ARGUMENT;
    }
    *out_pair_count = 0U;

    while ((pos < data_length) && ((data[pos] == ' ') || (data[pos] == '\t') ||
           (data[pos] == '\r') || (data[pos] == '\n')))
    {
        pos++;
    }
    if ((pos >= data_length) || (data[pos] != '{'))
    {
        return SERVER_JSON_SYNTAX_ERROR;
    }
    pos++;

    while (1)
    {
        while ((pos < data_length) && ((data[pos] == ' ') || (data[pos] == '\t') ||
               (data[pos] == '\r') || (data[pos] == '\n')))
        {
            pos++;
        }
        if ((pos < data_length) && (data[pos] == '}'))
        {
            if (close_allowed == 0U)
            {
                return SERVER_JSON_SYNTAX_ERROR;
            }
            pos++;
            break;
        }
        if ((pos >= data_length) || (data[pos] != '"'))
        {
            return SERVER_JSON_SYNTAX_ERROR;
        }
        if (pair_count >= max_pairs)
        {
            return SERVER_JSON_OUTPUT_FULL;
        }

        pos++;
        write_pos = 0U;
        escaped = 0U;
        while (pos < data_length)
        {
            if ((escaped == 0U) && (data[pos] == '"'))
            {
                break;
            }
            if ((data[pos] < 0x20U) || (write_pos >= (uint16_t)(key_stride - 1U)))
            {
                return (write_pos >= (uint16_t)(key_stride - 1U)) ?
                       SERVER_JSON_OUTPUT_FULL : SERVER_JSON_SYNTAX_ERROR;
            }
            keys[(uint16_t)pair_count * key_stride + write_pos++] = data[pos];
            escaped = ((escaped == 0U) && (data[pos] == '\\')) ? 1U : 0U;
            pos++;
        }
        if (pos >= data_length)
        {
            return SERVER_JSON_SYNTAX_ERROR;
        }
        keys[(uint16_t)pair_count * key_stride + write_pos] = '\0';
        pos++;

        while ((pos < data_length) && ((data[pos] == ' ') || (data[pos] == '\t') ||
               (data[pos] == '\r') || (data[pos] == '\n')))
        {
            pos++;
        }
        if ((pos >= data_length) || (data[pos] != ':'))
        {
            return SERVER_JSON_SYNTAX_ERROR;
        }
        pos++;

        while ((pos < data_length) && ((data[pos] == ' ') || (data[pos] == '\t') ||
               (data[pos] == '\r') || (data[pos] == '\n')))
        {
            pos++;
        }
        if (pos >= data_length)
        {
            return SERVER_JSON_SYNTAX_ERROR;
        }
        if ((data[pos] == '{') || (data[pos] == '['))
        {
            return SERVER_JSON_UNSUPPORTED_VALUE;
        }

        quoted_value = (data[pos] == '"') ? 1U : 0U;
        if (quoted_value != 0U)
        {
            pos++;
        }
        write_pos = 0U;
        escaped = 0U;
        while (pos < data_length)
        {
            if ((quoted_value != 0U) && (escaped == 0U) && (data[pos] == '"'))
            {
                break;
            }
            if ((quoted_value == 0U) && ((data[pos] == ',') || (data[pos] == '}')))
            {
                break;
            }
            if ((data[pos] < 0x20U) || (write_pos >= (uint16_t)(value_stride - 1U)))
            {
                return (write_pos >= (uint16_t)(value_stride - 1U)) ?
                       SERVER_JSON_OUTPUT_FULL : SERVER_JSON_SYNTAX_ERROR;
            }
            values[(uint16_t)pair_count * value_stride + write_pos++] = data[pos];
            escaped = ((escaped == 0U) && (data[pos] == '\\')) ? 1U : 0U;
            pos++;
        }
        if ((pos >= data_length) || ((quoted_value != 0U) && (data[pos] != '"')))
        {
            return SERVER_JSON_SYNTAX_ERROR;
        }
        values[(uint16_t)pair_count * value_stride + write_pos] = '\0';
        if (quoted_value != 0U)
        {
            pos++;
        }
        else
        {
            while ((write_pos > 0U) &&
                   ((values[(uint16_t)pair_count * value_stride + write_pos - 1U] == ' ') ||
                    (values[(uint16_t)pair_count * value_stride + write_pos - 1U] == '\t')))
            {
                values[(uint16_t)pair_count * value_stride + --write_pos] = '\0';
            }
            if (write_pos == 0U)
            {
                return SERVER_JSON_SYNTAX_ERROR;
            }
        }

        pair_count++;
        close_allowed = 1U;
        while ((pos < data_length) && ((data[pos] == ' ') || (data[pos] == '\t') ||
               (data[pos] == '\r') || (data[pos] == '\n')))
        {
            pos++;
        }
        if ((pos < data_length) && (data[pos] == ','))
        {
            pos++;
            close_allowed = 0U;
        }
        else if ((pos < data_length) && (data[pos] == '}'))
        {
            pos++;
            break;
        }
        else
        {
            return SERVER_JSON_SYNTAX_ERROR;
        }
    }

    while ((pos < data_length) && ((data[pos] == ' ') || (data[pos] == '\t') ||
           (data[pos] == '\r') || (data[pos] == '\n')))
    {
        pos++;
    }
    if (pos != data_length)
    {
        return SERVER_JSON_SYNTAX_ERROR;
    }
    *out_pair_count = pair_count;
    return SERVER_JSON_OK;
}

/* 使用指定分隔符拆分简单协议数据，例如 ':' 或 ',' 分隔的数据帧。 */
SERVER_JSON_State_e SERVER_JSON_SplitByDelimiter(const uint8_t *data,
                                                  uint16_t data_length,
                                                  uint8_t delimiter,
                                                  uint8_t *fields,
                                                  uint16_t field_stride,
                                                  uint8_t max_fields,
                                                  uint8_t *out_field_count)
{
    uint16_t pos = 0U;
    uint16_t write_pos;
    uint8_t field_count = 0U;

    if ((data == NULL) || (data_length == 0U) || (fields == NULL) ||
        (field_stride < 2U) || (max_fields == 0U) || (out_field_count == NULL))
    {
        return SERVER_JSON_INVALID_ARGUMENT;
    }
    *out_field_count = 0U;

    while (1)
    {
        if (field_count >= max_fields)
        {
            return SERVER_JSON_OUTPUT_FULL;
        }
        write_pos = 0U;
        while ((pos < data_length) && (data[pos] != delimiter))
        {
            if (write_pos >= (uint16_t)(field_stride - 1U))
            {
                return SERVER_JSON_OUTPUT_FULL;
            }
            fields[(uint16_t)field_count * field_stride + write_pos++] = data[pos++];
        }
        fields[(uint16_t)field_count * field_stride + write_pos] = '\0';
        field_count++;
        if (pos >= data_length)
        {
            break;
        }
        pos++;
    }

    *out_field_count = field_count;
    return SERVER_JSON_OK;
}
