#include "helpers/dronecan_communication.h"
#include "helpers/dronecan_value_params.h"
#include "messages/uavcan.protocol.param.GetSet-11.h"
#include <string.h>

// TODO move everything out that is not message specific, so that it can be reused in other messages if needed

uint8_t type_of_values(union DeviceParameter *param)
{
    return param->Float.type;
}

void *current_value(union DeviceParameter *param)
{
    switch (type_of_values(param))
    {
    case DEVICE_PARAM_TYPE_INT:
        return &param->Integer.value;
        break;
    case DEVICE_PARAM_TYPE_FLOAT:
        return &param->Float.value;
        break;
    case DEVICE_PARAM_TYPE_BOOL:
        return &param->Boolean.value;
        break;
    case DEVICE_PARAM_TYPE_STRING:
        return param->String.value;
        break;
    default:
        return NULL;
    }
}

void *default_value(union DeviceParameter *param)
{
    switch (type_of_values(param))
    {
    case DEVICE_PARAM_TYPE_INT:
        return &param->Integer.default_val;
        break;
    case DEVICE_PARAM_TYPE_FLOAT:
        return &param->Float.default_val;
        break;
    case DEVICE_PARAM_TYPE_BOOL:
        return &param->Boolean.default_val;
        break;
    case DEVICE_PARAM_TYPE_STRING:
        return param->String.default_val;
        break;
    default:
        return NULL;
    }
}

void *max_value(union DeviceParameter *param)
{
    switch (type_of_values(param))
    {
    case DEVICE_PARAM_TYPE_INT:
        return &param->Integer.max;
        break;
    case DEVICE_PARAM_TYPE_FLOAT:
        return &param->Float.max;
        break;
    default:
        return NULL;
    }
}

void *min_value(union DeviceParameter *param)
{
    switch (type_of_values(param))
    {
    case DEVICE_PARAM_TYPE_INT:
        return &param->Integer.min;
        break;
    case DEVICE_PARAM_TYPE_FLOAT:
        return &param->Float.min;
        break;
    default:
        return NULL;
    }
}

char *name_value(union DeviceParameter *param)
{
    switch (type_of_values(param))
    {
    case DEVICE_PARAM_TYPE_INT:
        return param->Integer.name;
        break;
    case DEVICE_PARAM_TYPE_FLOAT:
        return param->Float.name;
        break;
    case DEVICE_PARAM_TYPE_BOOL:
        return param->Boolean.name;
        break;
    case DEVICE_PARAM_TYPE_STRING:
        return param->String.name;
        break;
    default:
        return NULL;
    }
}

void set_value(union DeviceParameter *param, void *value)
{
    switch (type_of_values(param))
    {
    case DEVICE_PARAM_TYPE_INT:
        param->Integer.value = *(int64_t *)value;
        break;
    case DEVICE_PARAM_TYPE_FLOAT:
        param->Float.value = *(float *)value;
        break;
    case DEVICE_PARAM_TYPE_BOOL:
        param->Boolean.value = *(uint8_t *)value;
        break;
    case DEVICE_PARAM_TYPE_STRING:
        if (param->String.value != NULL)
        {
            free(param->String.value);
        }
        param->String.value = malloc(strlen((char *)value) + 1);
        strcpy(param->String.value, (char *)value);
        break;
    default:
        break;
    }
}

// return size in bits of filled data
uint16_t setNumericValue(void *destination, uint32_t bit_offset, uint16_t type, void *value)
{

    switch (type)
    {
    case DEVICE_PARAM_TYPE_INT:
        canardEncodeScalar(destination, bit_offset, 2, &type);
        canardEncodeScalar(destination, bit_offset + 2, 64, value);
        return 64 + 2;
        break;
    case DEVICE_PARAM_TYPE_FLOAT:
        canardEncodeScalar(destination, bit_offset, 2, &type);
        canardEncodeScalar(destination, bit_offset + 2, 32, value);
        return 32 + 2;
        break;
    default:
        return 2;
    }
}

// return size in bits of filled data
uint16_t setValue(void *destination, uint32_t bit_offset, uint16_t type, void *value)
{
    canardEncodeScalar(destination, bit_offset, 3, &type);
    switch (type)
    {
    case DEVICE_PARAM_TYPE_INT:
        canardEncodeScalar(destination, bit_offset + 3, 64, value);
        return 64 + 3;
        break;
    case DEVICE_PARAM_TYPE_FLOAT:
        canardEncodeScalar(destination, bit_offset + 3, 32, value);
        return 32 + 3;
        break;
    case DEVICE_PARAM_TYPE_BOOL:
        canardEncodeScalar(destination, bit_offset + 3, 8, value);
        return 8 + 3;
        break;
    case DEVICE_PARAM_TYPE_STRING:
        uint16_t strlength = strlen((char *)value);
        canardEncodeScalar(destination, bit_offset + 3, 8, &strlength);
        for (uint8_t i = 0; i < strlength; i++)
        {
            canardEncodeScalar(destination, bit_offset + 3 + 8 + i * 8, 8, &((char *)value)[i]);
        }
        return 3 + 8 + strlength * 8; // string + 3 for type + 8 string length
        break;
    default:
        return 3;
    }
}

uint16_t buffer_size(union DeviceParameter *data)
{
    uint16_t size = 4; // 4 fields for type of value, default value type, max value type and min value type

    switch (type_of_values(data))
    {
    case DEVICE_PARAM_TYPE_INT:
        size += 8 * 4; // value + default + max + min
        break;
    case DEVICE_PARAM_TYPE_FLOAT:
        size += 4 * 4; // value + default + max + min
        break;
    case DEVICE_PARAM_TYPE_BOOL:
        size += 1 * 2; // value + default
        break;
    case DEVICE_PARAM_TYPE_STRING:
        size += 1;                           // string length for value
        size += strlen(current_value(data)); // value string
        size += 1;                           // string length for default
        size += strlen(default_value(data)); // default string
        break;
    default:
        break;
    }
    return size + strlen(name_value(data)); // name string
}

bool response_11_paramGetSet(uint8_t destination_node_id, uint8_t *inout_transfer_id, union DeviceParameter *data)
{
    size_t size = buffer_size(data);
    uint8_t buffer[size];
    memset(buffer, 0, size);
    uint16_t offset = 0;

    offset += 5;
    offset += setValue(buffer, offset, type_of_values(data), current_value(data));
    offset += 5;
    offset += setValue(buffer, offset, type_of_values(data), default_value(data));
    offset += 6;
    offset += setNumericValue(buffer, offset, type_of_values(data), max_value(data));
    offset += 6;
    offset += setNumericValue(buffer, offset, type_of_values(data), min_value(data));

    char *name = (char *)name_value(data);
    uint8_t name_len = strlen(name);
    for (int i = 0; i < name_len; i++)
    {
        canardEncodeScalar(buffer, offset, 8, &name[i]);
        offset += 8;
    }

    return dronecan_respond(destination_node_id,
                            inout_transfer_id,
                            UAVCAN_PARAM_GETSET_SIGNATURE,
                            UAVCAN_PARAM_GETSET_ID,
                            CANARD_TRANSFER_PRIORITY_LOW,
                            buffer,
                            size);
}

bool response_11_paramGetSetEmpty(uint8_t destination_node_id, uint8_t *inout_transfer_id)
{
    uint8_t buffer[4] = {0};

    return dronecan_respond(destination_node_id,
                            inout_transfer_id,
                            UAVCAN_PARAM_GETSET_SIGNATURE,
                            UAVCAN_PARAM_GETSET_ID,
                            CANARD_TRANSFER_PRIORITY_LOW,
                            buffer,
                            sizeof(buffer));
}

uint16_t set_parameter_value(CanardRxTransfer *transfer, uint16_t name_start, void *value)
{
    // get name
    char name[transfer->payload_len - name_start / 8 + 1];

    for (int i = 0; i < transfer->payload_len - name_start / 8; i++)
    {
        canardDecodeScalar(transfer, name_start + i * 8, 8, false, &name[i]);
    }
    name[transfer->payload_len - name_start / 8] = '\0';
    // find parameter and set value
    for (int i = 0; i < get_device_parameters_len(); i++)
    {
        union DeviceParameter *parameter = &get_device_parameters()[i];
        if (strcmp(name_value(parameter), name) == 0)
        {
            if (value != NULL)
            {
                set_value(parameter, value);
                if (get_param_changed_callback() != NULL)
                {
                    get_param_changed_callback()(i);
                }
            }
            return i;
        }
    }
    return -1;
}

// for requested parameter return index, if value sent save it to parameter
uint16_t decode_11_paramGetSet_request(CanardRxTransfer *transfer)
{
    uint16_t param_index = 0;
    canardDecodeScalar(transfer, 0, 13, false, &param_index);
    uint8_t value_type;
    canardDecodeScalar(transfer, 13, 3, false, &value_type);
    uint16_t offset = 16; // index + value type

    if (value_type == DEVICE_PARAM_TYPE_EMPTY)
    {
        if (transfer->payload_len * 8 > offset) // if there is value in transfer, set it, otherwise just return index
        {
            return set_parameter_value(transfer, offset, NULL);
        }
        return param_index;
    }
    else if (value_type == DEVICE_PARAM_TYPE_INT || value_type == DEVICE_PARAM_TYPE_FLOAT || value_type == DEVICE_PARAM_TYPE_BOOL)
    {
        uint64_t int_value;
        canardDecodeScalar(transfer, offset, 64, true, &int_value);
        switch (value_type)
        {
        case DEVICE_PARAM_TYPE_INT:
            return set_parameter_value(transfer, offset + 64, &int_value);
        case DEVICE_PARAM_TYPE_FLOAT:
            return set_parameter_value(transfer, offset + 32, &int_value);
        case DEVICE_PARAM_TYPE_BOOL:
            return set_parameter_value(transfer, offset + 8, &int_value);
        default:
            return -1;
        }
    }
    else if (value_type == DEVICE_PARAM_TYPE_STRING)
    {
        uint8_t strlength;
        canardDecodeScalar(transfer, offset, 8, false, &strlength);
        offset += 8;
        char str_value[strlength + 1];
        for (int i = 0; i < strlength; i++)
        {
            canardDecodeScalar(transfer, offset, 8, false, &str_value[i]);
            offset += 8;
        }
        str_value[strlength] = '\0';
        return set_parameter_value(transfer, offset, str_value);
    }
    return -1; // unknown type
}
