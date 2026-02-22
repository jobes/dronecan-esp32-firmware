#include "app_tasks.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "messages/uavcan.protocol.NodeStatus-341.h"
#include "helpers/dronecan_value_params.h"

static void dronecan_spin_task(void *arg)
{
    ESP_LOGI("Main", "Spin task started");

    while (1)
    {
        dronecan_spin();
        vTaskDelay(pdMS_TO_TICKS(CAN_READ_TIMEOUT_MS));
    }
}

static void heartbeat_task(void *arg)
{
    ESP_LOGI("Main", "Heartbeat task started");

    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        publish_341_nodeStatus();
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(DRONECAN_HEARTBEAT_INTERVAL_MS)); // 1 second interval with some margin
    }
}

void static init_device_parameters()
{
    union DeviceParameter *device_parameters = get_device_parameters();
    uint16_t device_parameters_len = get_device_parameters_len();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t nvs_handle;
    err = nvs_open("devParCan", NVS_READWRITE, &nvs_handle);
    ESP_ERROR_CHECK(err);

    for (uint16_t i = 0; i < device_parameters_len; i++)
    {
        union DeviceParameter *param = &device_parameters[i];

        switch (param->Float.type)
        {
        case DEVICE_PARAM_TYPE_FLOAT:
            err = nvs_get_i16(nvs_handle, param->Float.name, (int16_t *)&param->Float.value);
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
            err = nvs_get_i16(nvs_handle, param->Boolean.name, (int16_t *)&param->Boolean.value);
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
            err = nvs_get_str(nvs_handle, param->String.name, NULL, &required_size);
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                param->String.value = malloc(strlen(param->String.default_val) + 1);
                strcpy(param->String.value, param->String.default_val);
            }
            else if (err == ESP_OK)
            {
                param->String.value = malloc(required_size);
                ESP_ERROR_CHECK(nvs_get_str(nvs_handle, param->String.name, param->String.value, &required_size));
            }
            else
            {
                ESP_ERROR_CHECK(err);
            }
            break;

        case DEVICE_PARAM_TYPE_INT:
            err = nvs_get_i16(nvs_handle, param->Integer.name, (int16_t *)&param->Integer.value);
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

void init_app(TaskFunction_t app_task)
{
    init_device_parameters();

    ESP_LOGI("Main", "========================================");
    ESP_LOGI("Main", "DroneCAN Node Starting...");
    ESP_LOGI("Main", "========================================");

    dronecan_init();

    xTaskCreate(
        dronecan_spin_task,
        "dronecan_spin", // read, write, dronecan operator
        4096,
        NULL,
        10,
        NULL);

    xTaskCreate(
        heartbeat_task,
        "dronecan_heartbeat",
        2048,
        NULL,
        5,
        NULL);

    xTaskCreate(
        app_task,
        "app_task",
        2048,
        NULL,
        3,
        NULL);

    ESP_LOGI("Main", "All tasks created successfully");
}