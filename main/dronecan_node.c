#include "dronecan_node.h"
#include "canard.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_mac.h"
#include "messages/uavcan.protocol.GetNodeInfo-1.h"
#include "messages/uavcan.protocol.RestartNode-5.h"
#include "messages/uavcan.protocol.param.GetSet-11.h"
#include "helpers/dronecan_value_params.h"

static const char *TAG = "DroneCAN";

static CanardInstance g_canard;
static uint8_t g_canard_memory_pool[DRONECAN_MEM_POOL_SIZE];

static SemaphoreHandle_t g_canard_mutex;
static uint8_t node_health = HEALTH_OK;
static uint8_t node_mode = MODE_INITIALIZATION;

union DeviceParameter device_parameters[] = {
    {.Float = {DEVICE_PARAM_TYPE_FLOAT, "temp_offset", 2.4, -18.2, 47.3, 3.16}},
    {.Boolean = {DEVICE_PARAM_TYPE_BOOL, "enable_temperature", 1, 0}},
    {.String = {DEVICE_PARAM_TYPE_STRING, "password", "current", "default"}},
    {.Integer = {DEVICE_PARAM_TYPE_INT, "frequency", 48, -21, 87, 13}}};
uint8_t device_parameters_len = ARRAY_SIZE(device_parameters);

static bool can_driver_init()
{
    twai_mode_t mode = CAN_MODE;
    twai_timing_config_t t_config = CAN_SPEED();
    twai_filter_config_t f_config = CAN_CONFIG();
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, mode);

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to install TWAI driver");
        return false;
    }

    if (twai_start() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start TWAI driver");
        return false;
    }

    ESP_LOGI(TAG, "CAN driver initialized");
    return true;
}

static bool can_transmit(const CanardCANFrame *frame)
{
    twai_message_t message = {0};

    message.identifier = frame->id & CANARD_CAN_EXT_ID_MASK;
    message.extd = 1;
    message.data_length_code = frame->data_len;
    memcpy(message.data, frame->data, frame->data_len);

    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(10));
    return (result == ESP_OK);
}

static bool can_receive(CanardCANFrame *frame)
{
    twai_message_t message;

    if (twai_receive(&message, pdMS_TO_TICKS(1)) != ESP_OK)
    {
        return false;
    }

    frame->id = message.identifier | CANARD_CAN_FRAME_EFF;
    frame->data_len = message.data_length_code;
    memcpy(frame->data, message.data, message.data_length_code);

    return true;
}

static bool should_accept_transfer(
    const CanardInstance *ins,
    uint64_t *out_data_type_signature,
    uint16_t data_type_id,
    CanardTransferType transfer_type,
    uint8_t source_node_id)
{
    (void)ins;
    (void)source_node_id;

    if (transfer_type == CanardTransferTypeRequest)
    {
        switch (data_type_id)
        {
        case UAVCAN_GET_NODE_INFO_ID:
            *out_data_type_signature = UAVCAN_GET_NODE_INFO_SIGNATURE;
            return true;
        case UAVCAN_RESTART_NODE_ID:
            *out_data_type_signature = UAVCAN_RESTART_NODE_SIGNATURE;
            return true;
        case UAVCAN_PARAM_GETSET_ID:
            *out_data_type_signature = UAVCAN_PARAM_GETSET_SIGNATURE;
            return true;
        }
    }

    return false;
}

static void on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer)
{
    if (transfer->transfer_type == CanardTransferTypeRequest)
    {
        switch (transfer->data_type_id)
        {
        case UAVCAN_GET_NODE_INFO_ID:
            response_1_getNodeInfo(transfer->source_node_id, &transfer->transfer_id);
            break;
        case UAVCAN_RESTART_NODE_ID:
            if (check_response_5_restart_transfer_valid(transfer))
            {
                response_5_restartNode(transfer->source_node_id, &transfer->transfer_id);
                vTaskDelay(pdMS_TO_TICKS(100)); // Give some time for the response to be sent before restarting
                esp_restart();
            }
            break;
        case UAVCAN_PARAM_GETSET_ID:
            uint16_t param_index = 0;
            canardDecodeScalar(transfer, 0, 13, false, &param_index);
            ESP_LOGI("GETSET", "index %d", param_index);
            if (param_index < device_parameters_len)
            {
                response_11_paramGetSet(transfer->source_node_id, &transfer->transfer_id, &device_parameters[param_index]);
            }
            else
            {
                response_11_paramGetSetEmpty(transfer->source_node_id, &transfer->transfer_id);
            }
            break;
        default:
            break;
        }
    }
}

void dronecan_spin()
{
    xSemaphoreTake(g_canard_mutex, portMAX_DELAY);

    const CanardCANFrame *frame;
    while ((frame = canardPeekTxQueue(&g_canard)) != NULL)
    {
        if (can_transmit(frame))
        {
            canardPopTxQueue(&g_canard);
        }
        else
        {
            break;
        }
    }

    CanardCANFrame rx_frame;
    while (can_receive(&rx_frame))
    {
        canardHandleRxFrame(&g_canard, &rx_frame, xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);
    }

    canardCleanupStaleTransfers(&g_canard, xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);

    xSemaphoreGive(g_canard_mutex);
}

void dronecan_init()
{
    ESP_LOGI(TAG, "Initializing DroneCAN node...");

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
               on_transfer_received,
               should_accept_transfer,
               NULL);

    canardSetLocalNodeID(&g_canard, DRONECAN_NODE_ID);

    ESP_LOGI(TAG, "DroneCAN initialized with Node ID: %d", DRONECAN_NODE_ID);
}

void set_node_health(uint8_t new_health)
{
    node_health = new_health;
}

void set_node_mode(uint8_t new_mode)
{
    node_mode = new_mode;
}

uint8_t *get_node_health()
{
    return &node_health;
}

uint8_t *get_node_mode()
{
    return &node_mode;
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
        ESP_LOGE(TAG, "Broadcast failed: %d", result);
        return false;
    }

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
        ESP_LOGE(TAG, "Respond failed: %d", result);
        return false;
    }

    return true;
}

// TODO parameter getters and setters; uavcan.protocol.param.ExecuteOpcode and uavcan.protocol.param.GetSet
// TODO node should not be initialized until it gets ID, and main APP should not do anything until then - no sending pressure
// TODO rewrite to a library style, main.c show then have only app task.
// TODO turn off wifi and bluetooth and everything not needed for the node to save power
// TODO dronecan.uavcan.protocol.gettransportstats
// TODO FW updater
