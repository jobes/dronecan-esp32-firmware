#include "dronecan_value_params.h"
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define NVS_NAMESPACE "devParCan"
static union DeviceParameter *singleton_device_parameters = 0;
static uint16_t singleton_device_parameters_len = 0;
static ParamChangedFunction param_changed_callback = 0;
static nvs_handle_t nvs_singleton_handle = 0;

void init_nvs()
{

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_singleton_handle);
    ESP_ERROR_CHECK(err);
}

void load_parameters_from_nvs()
{
    esp_err_t err = ESP_OK;
    union DeviceParameter *device_parameters = get_device_parameters();
    uint16_t device_parameters_len = get_device_parameters_len();

    for (uint16_t i = 0; i < device_parameters_len; i++)
    {
        union DeviceParameter *param = &device_parameters[i];
        switch (param->Float.type)
        {
        case DEVICE_PARAM_TYPE_FLOAT:
            size_t floatSize = sizeof(float);
            err = nvs_get_blob(nvs_singleton_handle, param->Float.name, &param->Float.value, &floatSize);
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                param->Float.value = param->Float.default_val;
            }
            else
            {
                ESP_ERROR_CHECK(err);
            }
            break;
        case DEVICE_PARAM_TYPE_BOOL:
            err = nvs_get_u8(nvs_singleton_handle, param->Boolean.name, &param->Boolean.value);
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                param->Boolean.value = param->Boolean.default_val;
            }
            else
            {
                ESP_ERROR_CHECK(err);
            }
            break;
        case DEVICE_PARAM_TYPE_STRING:
            size_t required_size;
            err = nvs_get_str(nvs_singleton_handle, param->String.name, NULL, &required_size);
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                param->String.value = malloc(strlen(param->String.default_val) + 1);
                strcpy(param->String.value, param->String.default_val);
            }
            else if (err == ESP_OK)
            {
                param->String.value = malloc(required_size);
                ESP_ERROR_CHECK(nvs_get_str(nvs_singleton_handle, param->String.name, param->String.value, &required_size));
            }
            else
            {
                ESP_ERROR_CHECK(err);
            }
            break;

        case DEVICE_PARAM_TYPE_INT:
            err = nvs_get_i64(nvs_singleton_handle, param->Integer.name, &param->Integer.value);
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                param->Integer.value = param->Integer.default_val;
            }
            else
            {
                ESP_ERROR_CHECK(err);
            }
            break;
        default:
            break;
        }
    }
}

void set_device_parameters(union DeviceParameter *device_parameters, uint16_t device_parameters_len)
{
    if (nvs_singleton_handle == 0)
    {
        init_nvs();
    }
    if (singleton_device_parameters != 0)
    {
        free(singleton_device_parameters);
    }
    singleton_device_parameters = malloc(sizeof(union DeviceParameter) * device_parameters_len);
    memcpy(singleton_device_parameters, device_parameters, sizeof(union DeviceParameter) * device_parameters_len);
    singleton_device_parameters_len = device_parameters_len;

    load_parameters_from_nvs();
}

union DeviceParameter *get_device_parameters()
{
    return singleton_device_parameters;
}

uint16_t get_device_parameters_len()
{
    return singleton_device_parameters_len;
}

union DeviceParameter *get_device_parameter(char *name)
{
    union DeviceParameter *device_parameters = get_device_parameters();
    uint16_t device_parameters_len = get_device_parameters_len();

    for (uint16_t i = 0; i < device_parameters_len; i++)
    {
        if (strcmp(device_parameters[i].Float.name, name) == 0)
        {
            return &device_parameters[i];
        }
    }
    return NULL;
}

void set_param_changed_callback(ParamChangedFunction callback)
{
    param_changed_callback = callback;
}

ParamChangedFunction get_param_changed_callback()
{
    return param_changed_callback;
}

bool process_parameters_to_nvs(enum OpcodeAction action)
{
    if (action == OPCODE_SAVE)
    {
        return save_parameters_to_nvs();
    }
    else if (action == OPCODE_ERASE)
    {
        return erase_parameters_from_nvs();
    }
    return false;
}

bool save_parameters_to_nvs()
{
    union DeviceParameter *device_parameters = get_device_parameters();
    uint16_t device_parameters_len = get_device_parameters_len();

    for (uint16_t i = 0; i < device_parameters_len; i++)
    {
        union DeviceParameter *param = &device_parameters[i];
        switch (param->Float.type)
        {
        case DEVICE_PARAM_TYPE_FLOAT:
            if (param->Float.value == param->Float.default_val)
            {
                nvs_erase_key(nvs_singleton_handle, param->Float.name);
            }
            else
            {
                nvs_set_blob(nvs_singleton_handle, param->Float.name, &param->Float.value, sizeof(float));
            }
            break;
        case DEVICE_PARAM_TYPE_BOOL:
            if (param->Boolean.value == param->Boolean.default_val)
            {
                nvs_erase_key(nvs_singleton_handle, param->Boolean.name);
            }
            else
            {
                nvs_set_u8(nvs_singleton_handle, param->Boolean.name, param->Boolean.value);
            }
            break;
        case DEVICE_PARAM_TYPE_STRING:
            if (strcmp(param->String.value, param->String.default_val) == 0)
            {
                nvs_erase_key(nvs_singleton_handle, param->String.name);
            }
            else
            {
                nvs_set_str(nvs_singleton_handle, param->String.name, param->String.value);
            }
            break;

        case DEVICE_PARAM_TYPE_INT:
            if (param->Integer.value == param->Integer.default_val)
            {
                nvs_erase_key(nvs_singleton_handle, param->Integer.name);
            }
            else
            {
                nvs_set_i64(nvs_singleton_handle, param->Integer.name, param->Integer.value);
            }
            break;
        default:
            break;
        }
    }
    uint8_t result = nvs_commit(nvs_singleton_handle);
    return result == ESP_OK;
}

bool erase_parameters_from_nvs()
{
    nvs_erase_all(nvs_singleton_handle);
    nvs_commit(nvs_singleton_handle);
    load_parameters_from_nvs();
    return true;
}