#ifndef UAVCAN_PROTOCOL_PARAM_GETSET_11_H
#define UAVCAN_PROTOCOL_PARAM_GETSET_11_H

#include "dronecan_node.h"
#include "helpers/dronecan_value_params.h"

#define UAVCAN_PARAM_GETSET_ID 11
#define UAVCAN_PARAM_GETSET_SIGNATURE 0xA7B622F939D1A4D5ULL

static uint8_t type_of_values(union DeviceParameter *param)
{
    return param->Float.type;
}

static void *current_value(union DeviceParameter *param)
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

static void *default_value(union DeviceParameter *param)
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

static void *max_value(union DeviceParameter *param)
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

static void *min_value(union DeviceParameter *param)
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

static char *name_value(union DeviceParameter *param)
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

// return size in bits of filled data
static uint16_t setNumericValue(void *destination, uint32_t bit_offset, uint16_t type, void *value)
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
static uint16_t setValue(void *destination, uint32_t bit_offset, uint16_t type, void *value)
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

static uint16_t buffer_size(union DeviceParameter *data)
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

#endif // UAVCAN_PROTOCOL_PARAM_GETSET_11_H
