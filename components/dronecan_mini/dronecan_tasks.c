#include "dronecan_tasks.h"
#include "dronecan_accepter.h"
#include "dronecan_communication.h"
#include "dronecan_dna_receiver.h"
#include "dronecan_receiver.h"
#include "dronecan_value_params.h"
#include "esp_can.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "messages/uavcan.protocol.NodeStatus-341.h"

static const char *TAG = "TASKS";
static dronecan_frame_listener_t rx_listener = NULL;
static dronecan_frame_listener_t tx_listener = NULL;
static QueueHandle_t s_inject_queue = NULL;
static TaskHandle_t s_spin_task_handle = NULL;

void dronecan_set_rx_frame_listener(dronecan_frame_listener_t listener)
{
  rx_listener = listener;
}

void dronecan_set_tx_frame_listener(dronecan_frame_listener_t listener)
{
  tx_listener = listener;
}

void dronecan_inject_rx_frame(const CanardCANFrame *frame)
{
  if (s_inject_queue != NULL)
  {
    xQueueSend(s_inject_queue, frame, 0);
    if (s_spin_task_handle)
    {
      xTaskNotifyGive(s_spin_task_handle);
    }
  }
}

static void dronecan_spin_task(void *arg)
{
  ESP_LOGI(TAG, "Spin task started on core %d", xPortGetCoreID());

  while (1)
  {
    // Wait for notification or timeout (periodic CAN HW polling)
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(CAN_READ_TIMEOUT_MS));

    xSemaphoreTake(get_dronecan_communication_semaphore(), portMAX_DELAY);

    // 1. Drain canard TX queue
    const CanardCANFrame *frame;
    while ((frame = canardPeekTxQueue(get_dronecan_instance())) != NULL)
    {
      if (can_transmit(frame))
      {
        if (tx_listener != NULL)
        {
          tx_listener(frame);
        }
        canardPopTxQueue(get_dronecan_instance());
      }
    }

    // 2. CAN hardware RX
    CanardCANFrame rx_frame;
    while (can_receive(&rx_frame))
    {
      if (rx_listener != NULL)
      {
        rx_listener(&rx_frame);
      }
      int16_t result =
          canardHandleRxFrame(get_dronecan_instance(), &rx_frame,
                              xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);
      if (result != 0 && result != -CANARD_ERROR_RX_NOT_WANTED)
      {
        ESP_LOGI(TAG, "Error handling received frame: %d", result);
        increase_logical_error();
      }
    }

    // 3. Inject queue RX (frames from Cannelloni/UDP)
    if (s_inject_queue != NULL)
    {
      CanardCANFrame inject_frame;
      while (xQueueReceive(s_inject_queue, &inject_frame, 0) == pdPASS)
      {
        int16_t result = canardHandleRxFrame(
            get_dronecan_instance(), &inject_frame,
            xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);
        if (result != 0 && result != -CANARD_ERROR_RX_NOT_WANTED)
        {
          ESP_LOGI(TAG, "Error handling injected frame: %d", result);
          increase_logical_error();
        }
      }
    }

    // 4. Cleanup stale transfers
    canardCleanupStaleTransfers(get_dronecan_instance(),
                                xTaskGetTickCount() * portTICK_PERIOD_MS *
                                    1000);

    xSemaphoreGive(get_dronecan_communication_semaphore());
  }
}

static void heartbeat_task(void *arg)
{
  ESP_LOGI(TAG, "Heartbeat task started on core %d", xPortGetCoreID());

  TickType_t last_wake_time = xTaskGetTickCount();

  while (1)
  {
    uint32_t uptime = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
    publish_341_nodeStatus(uptime, get_node_health(), get_node_mode());
    vTaskDelayUntil(
        &last_wake_time,
        pdMS_TO_TICKS(DRONECAN_HEARTBEAT_INTERVAL_MS)); // 1 second interval
                                                        // with some margin
  }
}

void init_tasks(TaskFunction_t app_task, uint8_t node_id, BaseType_t core_id)
{
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "DroneCAN Node Starting...");
  ESP_LOGI(TAG, "========================================");

  s_inject_queue = xQueueCreate(32, sizeof(CanardCANFrame));

  dronecan_init(on_transfer_received, should_accept_transfer);

  xTaskCreatePinnedToCore(dronecan_spin_task,
                          "dronecan_spin", // read, write, dronecan operator
                          4096, NULL, 10, &s_spin_task_handle, core_id);

  if (node_id != 0)
  {
    set_node_id(node_id);
  }
  else
  {
    get_node_id();
    esp_ota_mark_app_valid_cancel_rollback();
  }

  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "DroneCAN UP WITH NODE_ID %d AND RUNNING.",
           canardGetLocalNodeID(get_dronecan_instance()));
  ESP_LOGI(TAG, "========================================");

  // after we got node ID, we can start other tasks, for example heartbeat and
  // app task
  xTaskCreatePinnedToCore(heartbeat_task, "dronecan_heartbeat", 2048, NULL, 5,
                          NULL, core_id);

  xTaskCreatePinnedToCore(app_task, "app_task", 2048, NULL, 3, NULL, core_id);

  ESP_LOGI(TAG, "All tasks created successfully (core_id=%d)",
           (int)core_id);
}