#include "helpers/dronecan_tasks.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "messages/uavcan.protocol.NodeStatus-341.h"
#include "helpers/dronecan_value_params.h"
#include "helpers/dronecan_communication.h"
#include "helpers/dronecan_dna_receiver.h"
#include "helpers/esp_can.h"
#include "helpers/dronecan_receiver.h"
#include "helpers/dronecan_accepter.h"

static const char *TAG = "TASKS";

static void dronecan_spin_task(void *arg)
{
    ESP_LOGI(TAG, "Spin task started");

    while (1)
    {
        xSemaphoreTake(get_dronecan_communication_semaphore(), portMAX_DELAY);

        const CanardCANFrame *frame;
        while ((frame = canardPeekTxQueue(get_dronecan_instance())) != NULL)
        {
            if (can_transmit(frame))
            {
                canardPopTxQueue(get_dronecan_instance());
            }
            else
            {
                break;
            }
        }

        CanardCANFrame rx_frame;
        while (can_receive(&rx_frame))
        {
            int16_t result = canardHandleRxFrame(get_dronecan_instance(), &rx_frame, xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);
            if (result != 0 && result != -CANARD_ERROR_RX_NOT_WANTED)
            {
                ESP_LOGI(TAG, "Error handling received frame: %d", result);
                increase_logical_error();
            }
        }

        canardCleanupStaleTransfers(get_dronecan_instance(), xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);

        xSemaphoreGive(get_dronecan_communication_semaphore());
        vTaskDelay(pdMS_TO_TICKS(CAN_READ_TIMEOUT_MS));
    }
}

static void heartbeat_task(void *arg)
{
    ESP_LOGI(TAG, "Heartbeat task started");

    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        publish_341_nodeStatus();
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(DRONECAN_HEARTBEAT_INTERVAL_MS)); // 1 second interval with some margin
    }
}

void init_tasks(TaskFunction_t app_task)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "DroneCAN Node Starting...");
    ESP_LOGI(TAG, "========================================");

    dronecan_init(on_transfer_received, should_accept_transfer);

    xTaskCreate(
        dronecan_spin_task,
        "dronecan_spin", // read, write, dronecan operator
        4096,
        NULL,
        10,
        NULL);

    get_node_id();
    esp_ota_mark_app_valid_cancel_rollback();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "DroneCAN UP WITH NODE_ID %d AND RUNNING.", get_node_id());
    ESP_LOGI(TAG, "========================================");

    // after we got node ID, we can start other tasks, for example heartbeat and app task
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

    ESP_LOGI(TAG, "All tasks created successfully");
}