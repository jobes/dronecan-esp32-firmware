#include "dronecan_mini/dronecan_tasks.h"
#include "esp_log.h"

#include "dronecan_mini/dronecan_value_params.h"
#include "dronecan_mini/dronecan_node_state.h"
#include "dronecan_mini/messages/uavcan.equipment.air_data.StaticPressure-1028.h"
#include "dronecan_mini/messages/uavcan.equipment.air_data.StaticTemperature-1029.h"
#include "wifi_manager.h"


static const char *TAG = "APP";

static void main_task(void *arg)
{
    ESP_LOGI(TAG, "Application task started");

    if (*get_node_mode() == MODE_INITIALIZATION)
    {
        set_node_mode(MODE_OPERATIONAL);
    }

    while (1)
    {
        if (*get_node_mode() != MODE_OPERATIONAL)
        {
            ESP_LOGW(TAG, "Node is not in operational mode, skipping sensor read and publish");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void param_changed(uint16_t index)
{
    ESP_LOGI(TAG, "Parameter index %d changed", index);
}

void app_main(void)
{
    union DeviceParameter device_parameters[] = {
        {.String = {DEVICE_PARAM_TYPE_STRING, "AP_SSID", "airplane_gateway"}},
        {.String = {DEVICE_PARAM_TYPE_STRING, "AP_PASSWORD", "airplane123456"}},
        {.String = {DEVICE_PARAM_TYPE_STRING, "STA_SSID", ""}},
        {.String = {DEVICE_PARAM_TYPE_STRING, "STA_PASSWORD", ""}},
    };

    set_param_changed_callback(param_changed);
    set_device_parameters(device_parameters, ARRAY_SIZE(device_parameters));
    wifi_init_manager();
    init_tasks(main_task);
}
