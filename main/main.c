#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dronecan_node.h"
#include "driver/gpio.h"
#include "bmp390.h"
#include "bmp3.h"
#include "messages/uavcan.equipment.air_data.StaticPressure-1028.h"

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
            float pressure_variance_pa2 = 0.0f; // Variance not provided by sensor driver yet
            bool pressure_publish_ok = publish_1028_staticPressure((float)data.pressure, pressure_variance_pa2);
            bool temperature_publish_ok = false;

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

void app_main(void)
{
    init_app(main_task);
}
