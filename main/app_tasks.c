#include "app_tasks.h"
#include "esp_log.h"
#include "messages/uavcan.protocol.NodeStatus-341.h"

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

void init_app(TaskFunction_t app_task)
{
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