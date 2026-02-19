#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dronecan_node.h"
#include "driver/gpio.h"

static const char *TAG = "Main";

static void dronecan_spin_task(void *arg)
{
    ESP_LOGI(TAG, "DroneCAN spin task started");

    while (1)
    {
        dronecan_spin();
        vTaskDelay(pdMS_TO_TICKS(CAN_READ_TIMEOUT_MS));
    }
}

static void heartbeat_task(void *arg)
{
    ESP_LOGI(TAG, "Heartbeat task started");

    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        dronecan_publish_node_status();
        ESP_LOGI(TAG, "Heartbeat published, uptime: %lu s", xTaskGetTickCount() * portTICK_PERIOD_MS / 1000);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(950)); // 1 second interval with some margin
    }
}

static void app_task(void *arg)
{
    ESP_LOGI(TAG, "Application task started");

    while (1)
    {
        gpio_set_level(GPIO_NUM_8, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(GPIO_NUM_8, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-C3 DroneCAN Node Starting...");
    ESP_LOGI(TAG, "========================================");

    dronecan_init();

    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);

    xTaskCreate(
        dronecan_spin_task,
        "dronecan_spin", // read, write, dronecan operator
        4096,
        NULL,
        10,
        NULL);

    xTaskCreate(
        heartbeat_task,
        "heartbeat",
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

    ESP_LOGI(TAG, "All tasks created successfully");
}
