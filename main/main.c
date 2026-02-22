#include "app_tasks.h"
#include "esp_log.h"

#include "bmp390.h"
#include "helpers/dronecan_value_params.h"
#include "messages/uavcan.equipment.air_data.StaticPressure-1028.h"
#include "messages/uavcan.equipment.air_data.StaticTemperature-1029.h"

static const char *TAG = "APP";

static void main_task(void *arg)
{
    ESP_LOGI(TAG, "Application task started");
    if (bmp390_spi_init() != ESP_OK)
    {
        set_node_health(HEALTH_CRITICAL);
        vTaskDelete(NULL);
    }

    set_node_mode(MODE_OPERATIONAL);

    while (1)
    {
        struct bmp3_data data;
        if (bmp390_get_data(&data) == BMP3_OK)
        {
            _Float16 pressure_variance_pa2 = 100.0f; // 0.8 meters variance
            _Float16 temperature_variance_c2 = 1.0f; // 1 degrees Celsius variance

                        bool pressure_publish_ok = publish_1028_staticPressure((_Float32)data.pressure + get_device_parameters()[0].Float.value, pressure_variance_pa2);
            bool temperature_publish_ok = publish_1029_staticTemperatureCelsius((_Float16)data.temperature + get_device_parameters()[1].Float.value, temperature_variance_c2);

            if (pressure_publish_ok && temperature_publish_ok)
            {
                set_node_health(HEALTH_OK);
            }
            else
            {
                if (temperature_publish_ok)
                {
                    ESP_LOGE(TAG, "Failed to publish static pressure");
                    set_node_health(HEALTH_CRITICAL);
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to publish temperature");
                    set_node_health(HEALTH_ERROR);
                }
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read data from BMP390 sensor");
            set_node_health(HEALTH_CRITICAL);
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
        {.Float = {DEVICE_PARAM_TYPE_FLOAT, "pressure_offset", -100.0, 100.0, 0.0}},
        {.Float = {DEVICE_PARAM_TYPE_FLOAT, "temp_offset", -10.0, 10.0, 0.0}},
    };

    set_param_changed_callback(param_changed);
    set_device_parameters(device_parameters, ARRAY_SIZE(device_parameters));
    init_app(main_task);
}
