#include "dronecan_value_params.h"
#include <stdlib.h>
#include <string.h>

static union DeviceParameter *singleton_device_parameters = 0;
static uint16_t singleton_device_parameters_len = 0;

void set_device_parameters(union DeviceParameter *device_parameters, uint16_t device_parameters_len)
{
    if (singleton_device_parameters != 0)
    {
        free(singleton_device_parameters);
    }
    singleton_device_parameters = malloc(sizeof(union DeviceParameter) * device_parameters_len);
    memcpy(singleton_device_parameters, device_parameters, sizeof(union DeviceParameter) * device_parameters_len);
    singleton_device_parameters_len = device_parameters_len;
}

union DeviceParameter *get_device_parameters()
{
    return singleton_device_parameters;
}

uint16_t get_device_parameters_len()
{
    return singleton_device_parameters_len;
}