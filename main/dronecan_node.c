#include "dronecan_node.h"
#include "canard.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_random.h"
#include <string.h>
#include "esp_mac.h"
#include "messages/uavcan.protocol.GetNodeInfo-1.h"
#include "messages/uavcan.protocol.RestartNode-5.h"
#include "messages/uavcan.protocol.param.GetSet-11.h"
#include "messages/uavcan.protocol.param.ExecuteOpcode-10.h"
#include "messages/uavcan.protocol.file.BeginFirmwareUpdate-40.h"
#include "messages/uavcan.protocol.file.Read-48.h"
#include "messages/uavcan.protocol.dynamic_node_id.Allocation-1.h"
#include "messages/uavcan.protocol.GetTransportStats-4.h"
#include "helpers/dronecan_value_params.h"

static const char *TAG = "DroneCAN";

static CanardInstance g_canard;
static uint8_t g_canard_memory_pool[DRONECAN_MEM_POOL_SIZE];

static SemaphoreHandle_t g_canard_mutex;
static uint8_t node_health = HEALTH_OK;
static uint8_t node_mode = MODE_INITIALIZATION;
static bool node_id_message_received = false; // another device is trying to allocate node ID, so wait for it to finish before start allocation process

static bool can_driver_init()
{
    twai_mode_t mode = CAN_MODE;
    twai_timing_config_t t_config = CAN_SPEED();
    twai_filter_config_t f_config = CAN_CONFIG();
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, mode);
    g_config.rx_queue_len = 128;

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
    if (result == ESP_OK)
    {
        increase_physical_tx();
    }
    return (result == ESP_OK);
}

static bool can_receive(CanardCANFrame *frame)
{
    twai_message_t message;

    if (twai_receive(&message, pdMS_TO_TICKS(1)) != ESP_OK)
    {
        return false;
    }
    increase_physical_rx();

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

    if (canardGetLocalNodeID(ins) == CANARD_BROADCAST_NODE_ID) // node 0 will work only with allocation
    {
        if (data_type_id == UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID)
        {
            *out_data_type_signature = UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE;
            return true;
        }
    }
    else if (transfer_type == CanardTransferTypeRequest)
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
        case UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_ID:
            *out_data_type_signature = UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_SIGNATURE;
            return true;
        case UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_ID:
            *out_data_type_signature = UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_SIGNATURE;
            return true;
        case UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID:
            *out_data_type_signature = UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_SIGNATURE;
            return true;
        }
    }
    else if (transfer_type == CanardTransferTypeResponse)
    {
        switch (data_type_id)
        {
        case UAVCAN_FILE_READ_ID:
            *out_data_type_signature = UAVCAN_FILE_READ_SIGNATURE;
            return true;
        }
    }
    return false;
}

static void on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer)
{
    increase_logical_rx();
    if (canardGetLocalNodeID(ins) == CANARD_BROADCAST_NODE_ID)
    {
        if (transfer->data_type_id == UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID)
        {
            ESP_LOGI(TAG, "Received dynamic node ID allocation transfer from node %d with length %d", transfer->source_node_id, transfer->payload_len);
            if (transfer->source_node_id == CANARD_BROADCAST_NODE_ID)
            {
                node_id_message_received = true;
                ESP_LOGW(TAG, "Another node is trying to allocate node ID, waiting for it to finish...");
            }
            else
            {
                if (transfer->payload_len < 17)
                {
                    return; // not enough data for node ID allocation, ignore
                }
                uint8_t node_id = process_1_dynamicNodeIdAllocation(transfer);
                if (node_id != 0)
                {
                    ESP_LOGI(TAG, "Setting node ID to %d", node_id);
                    canardSetLocalNodeID(ins, node_id);
                }
                else
                {
                    ESP_LOGW(TAG, "Received node id 0, try allocation again");
                    node_id_message_received = true;
                }
            }
        }
    }
    else if (transfer->transfer_type == CanardTransferTypeRequest)
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
            uint16_t param_index = decode_11_paramGetSet_request(transfer);

            if (param_index < get_device_parameters_len())
            {
                response_11_paramGetSet(transfer->source_node_id, &transfer->transfer_id, &get_device_parameters()[param_index]);
            }
            else
            {
                response_11_paramGetSetEmpty(transfer->source_node_id, &transfer->transfer_id);
            }
            break;
        case UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_ID:
            response_10_paramExecuteOpcode_process(transfer);
            break;
        case UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_ID:
            if (node_mode == MODE_SOFTWARE_UPDATE)
            {
                response_40_beginFirmwareUpdate(IN_PROGRESS, transfer->source_node_id, &transfer->transfer_id);
                break;
            }
            node_mode = MODE_SOFTWARE_UPDATE;
            if (process_40_beginFirmwareUpdate(transfer))
            {
                response_40_beginFirmwareUpdate(OK, transfer->source_node_id, &transfer->transfer_id);
                request_read_48(firmware_source_node_id, 0, firmware_path);
            }
            else
            {
                response_40_beginFirmwareUpdate(INVALID_MODE, transfer->source_node_id, &transfer->transfer_id);
            }

            break;
        case UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID:
            response_4_getTransportStats(transfer->source_node_id, &transfer->transfer_id);
            break;
        default:
            ESP_LOGW(TAG, "UNKNOWN TRANSFER RECEIVED: type_id=%d, source_node_id=%d", transfer->data_type_id, transfer->source_node_id);
            break;
        }
    }
    else if (transfer->transfer_type == CanardTransferTypeResponse)
    {
        switch (transfer->data_type_id)
        {
        case UAVCAN_FILE_READ_ID:
            if (node_mode == MODE_SOFTWARE_UPDATE)
            {
                firmware_file_chunk_received(transfer);
            }

            break;
        default:
            ESP_LOGW(TAG, "UNKNOWN RESPONSE RECEIVED: type_id=%d, source_node_id=%d", transfer->data_type_id, transfer->source_node_id);
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
        int16_t result = canardHandleRxFrame(&g_canard, &rx_frame, xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);
        if (result != 0 && result != -CANARD_ERROR_RX_NOT_WANTED)
        {
            ESP_LOGI(TAG, "Error handling received frame: %d", result);
            increase_logical_error();
        }
    }

    canardCleanupStaleTransfers(&g_canard, xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);

    xSemaphoreGive(g_canard_mutex);
}

void dronecan_init()
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
               on_transfer_received,
               should_accept_transfer,
               NULL);
}

void get_node_id()
{
    // wait second before starting node ID allocation process to give some time for other nodes to start and allocate ID if they want, so we can avoid conflicts
    // also, we should listen for a second, as some node can be already in process. And it is good to wait until server is ready when all nodes starts at the same time
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "DroneCAN node getting node ID started");
    while (canardGetLocalNodeID(&g_canard) == CANARD_BROADCAST_NODE_ID)
    {
        if (node_id_message_received)
        {
            // start initialization from start
            node_id_message_received = false;
            ESP_LOGI(TAG, "Node ID message received during initialization, node ID should be set soon");
            // random 400ms to 600ms delay to wait for node ID to be set, if not set, then start allocation process
            vTaskDelay(pdMS_TO_TICKS(400 + (esp_random() % 200)));
            continue;
        }
        if (!publish_1_dynamicNodeIdAllocation(0, FIRST_PART, get_unique_id()))
        {
            ESP_LOGE(TAG, "Failed to publish dynamic node ID allocation message (first part)");
            node_id_message_received = true;
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(100 + (esp_random() % 300)));
        if (node_id_message_received)
        {
            ESP_LOGI(TAG, "Node ID message received during initialization, node ID should be set soon");
            continue;
        }
        if (!publish_1_dynamicNodeIdAllocation(0, SECOND_PART, get_unique_id()))
        {
            ESP_LOGE(TAG, "Failed to publish dynamic node ID allocation message (second part)");
            node_id_message_received = true;
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(100 + (esp_random() % 300)));
        if (node_id_message_received)
        {
            ESP_LOGI(TAG, "Node ID message received during initialization, node ID should be set soon");
            continue;
        }
        if (!publish_1_dynamicNodeIdAllocation(0, FINAL_PART, get_unique_id()))
        {
            ESP_LOGE(TAG, "Failed to publish dynamic node ID allocation message (final part)");
            node_id_message_received = true;
            continue;
        }
        ESP_LOGI(TAG, "Dynamic node ID allocation message published, waiting for response...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
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
        increase_logical_error();
        ESP_LOGE(TAG, "Broadcast failed: %d", result);
        return false;
    }

    increase_logical_tx();
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
        increase_logical_error();
        ESP_LOGE(TAG, "Respond failed: %d", result);
        return false;
    }

    increase_logical_tx();
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
        increase_logical_error();
        ESP_LOGE(TAG, "Respond failed: %d", result);
        return false;
    }

    increase_logical_tx();
    return true;
}

// TODO rewrite restart so it really send message before restart, now it just wait and restart without guarantee that message is sent
// TODO rewrite to a library style, main.c show then have only app task.
// TODO remove all non needed static (move to .C)
