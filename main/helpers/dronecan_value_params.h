#ifndef DRONECAN_VALUE_PARAMS_H
#define DRONECAN_VALUE_PARAMS_H
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#include <stdint.h>
#include <stdbool.h>

enum DeviceParameterType
{
    DEVICE_PARAM_TYPE_EMPTY = 0,
    DEVICE_PARAM_TYPE_INT = 1,
    DEVICE_PARAM_TYPE_FLOAT = 2,
    DEVICE_PARAM_TYPE_BOOL = 3,
    DEVICE_PARAM_TYPE_STRING = 4
};
struct DeviceParameterFloat
{
    enum DeviceParameterType type;
    char *name;
    float value;
    float min;
    float max;
    float default_val;
};

struct DeviceParameterInteger
{
    enum DeviceParameterType type;
    char *name;
    int64_t value;
    int64_t min;
    int64_t max;
    int64_t default_val;
};

struct DeviceParameterBoolean
{
    enum DeviceParameterType type;
    char *name;
    uint8_t value;
    uint8_t default_val;
};

struct DeviceParameterString
{
    enum DeviceParameterType type;
    char *name;
    char *value;
    char *default_val;
};

union DeviceParameter
{
    struct DeviceParameterFloat Float;
    struct DeviceParameterInteger Integer;
    struct DeviceParameterBoolean Boolean;
    struct DeviceParameterString String;
};

void set_device_parameters(union DeviceParameter *device_parameters, uint16_t device_parameters_len);

union DeviceParameter *get_device_parameters();

uint16_t get_device_parameters_len();

#endif // DRONECAN_VALUE_PARAMS_H