#include "dronecan_communication.h"
#include "canard.h"
#include "esp_log.h"
#include "messages/uavcan.protocol.GetNodeInfo-1.h"
#include "helpers/esp_can.h"

static const char *TAG = "DC COMM"; // dronecan communication
static SemaphoreHandle_t g_canard_mutex;
static CanardInstance g_canard;
static uint8_t g_canard_memory_pool[DRONECAN_MEM_POOL_SIZE];
static uint64_t logical_error = 0;
static uint64_t logical_tx = 0;

void dronecan_init(CanardOnTransferReception on_reception, CanardShouldAcceptTransfer should_accept)
{
    ESP_LOGI(TAG, "Initializing DroneCAN node...");
    init_unique_id();

    g_canard_mutex = xSemaphoreCreateMutex();
    configASSERT(g_canard_mutex != NULL);

    if (!can_driver_init())
    {
        ESP_LOGE(TAG, "CAN driver init failed!");
        return;
    }

    canardInit(&g_canard,
               g_canard_memory_pool,
               sizeof(g_canard_memory_pool),
               on_reception,
               should_accept,
               NULL);
}

bool dronecan_broadcast(uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len, uint8_t *transfer_id)
{

    xSemaphoreTake(g_canard_mutex, portMAX_DELAY);

    int16_t result = canardBroadcast(&g_canard,
                                     signature,
                                     type_id,
                                     transfer_id,
                                     priority,
                                     payload,
                                     len);

    xSemaphoreGive(g_canard_mutex);

    if (result <= 0)
    {
        logical_error++;
        ESP_LOGE(TAG, "Broadcast failed: %d", result);
        return false;
    }

    logical_tx++;
    return true;
}

bool dronecan_respond(uint8_t destination_node_id, uint8_t *inout_transfer_id, uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len)
{
    int16_t result = canardRequestOrRespond(&g_canard,
                                            destination_node_id,
                                            signature,
                                            type_id,
                                            inout_transfer_id,
                                            priority,
                                            CanardResponse,
                                            payload,
                                            len);

    if (result <= 0)
    {
        logical_error++;
        ESP_LOGE(TAG, "Respond failed: %d", result);
        return false;
    }

    logical_tx++;
    return true;
}

bool dronecan_request(uint8_t destination_node_id, uint8_t *inout_transfer_id, uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len)
{
    int16_t result = canardRequestOrRespond(&g_canard,
                                            destination_node_id,
                                            signature,
                                            type_id,
                                            inout_transfer_id,
                                            priority,
                                            CanardRequest,
                                            payload,
                                            len);

    if (result <= 0)
    {
        logical_error++;
        ESP_LOGE(TAG, "Request failed: %d", result);
        return false;
    }

    logical_tx++;
    return true;
}

SemaphoreHandle_t get_dronecan_communication_semaphore()
{
    return g_canard_mutex;
}

CanardInstance *get_dronecan_instance()
{
    return &g_canard;
}

uint64_t get_logical_error()
{
    return logical_error;
}

void increase_logical_error()
{
    logical_error++;
}
