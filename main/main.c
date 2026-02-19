#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dronecan_node.h"

static const char *TAG = "Main";

extern volatile uint32_t g_uptime_sec;

static void dronecan_spin_task(void *arg)
{
    ESP_LOGI(TAG, "DroneCAN spin task started");

    while (1)
    {
        dronecan_spin();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void heartbeat_task(void *arg)
{
    ESP_LOGI(TAG, "Heartbeat task started");

    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        dronecan_publish_node_status();
        g_uptime_sec++;

        ESP_LOGI(TAG, "Heartbeat published, uptime: %lu s", g_uptime_sec);

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(950)); // 1 second interval with some margin
    }
}

static void app_task(void *arg)
{
    ESP_LOGI(TAG, "Application task started");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-C3 DroneCAN Node Starting...");
    ESP_LOGI(TAG, "========================================");

    dronecan_init();

    xTaskCreate(
        dronecan_spin_task,
        "dronecan_spin", // read, write, dronecan operator
        4096,
        NULL,
        configMAX_PRIORITIES - 1,
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
