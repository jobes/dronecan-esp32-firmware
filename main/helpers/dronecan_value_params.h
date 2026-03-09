#ifndef DRONECAN_VALUE_PARAMS_H
#define DRONECAN_VALUE_PARAMS_H
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#include <stdint.h>
#include <canard.h>

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
    float min;
    float max;
    float default_val;
    float value;
};

struct DeviceParameterInteger
{
    enum DeviceParameterType type;
    char *name;
    int64_t min;
    int64_t max;
    int64_t default_val;
    int64_t value;
};

struct DeviceParameterBoolean
{
    enum DeviceParameterType type;
    char *name;
    uint8_t default_val;
    uint8_t value;
};

struct DeviceParameterString
{
    enum DeviceParameterType type;
    char *name;
    char *default_val;
    char *value;
};

union DeviceParameter
{
    struct DeviceParameterFloat Float;
    struct DeviceParameterInteger Integer;
    struct DeviceParameterBoolean Boolean;
    struct DeviceParameterString String;
};

typedef void (*ParamChangedFunction)(uint16_t index);

void set_param_changed_callback(ParamChangedFunction callback);
ParamChangedFunction get_param_changed_callback();

void set_device_parameters(union DeviceParameter *device_parameters, uint16_t device_parameters_len);
union DeviceParameter *get_device_parameters();
uint16_t get_device_parameters_len();
bool save_parameters_to_nvs();
bool erase_parameters_from_nvs();

#endif // DRONECAN_VALUE_PARAMS_H