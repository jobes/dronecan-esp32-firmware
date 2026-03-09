#include "canard.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "helpers/dronecan_communication.h"
#include "esp_random.h"
#include "messages/uavcan.protocol.GetNodeInfo-1.h"
#include "messages/uavcan.protocol.dynamic_node_id.Allocation-1.h"

static const char *TAG = "DroneCAN DNA receiver";

static bool node_id_message_received = false; // another device is trying to allocate node ID, so wait for it to finish before start allocation process

void get_node_id()
{
    // wait second before starting node ID allocation process to give some time for other nodes to start and allocate ID if they want, so we can avoid conflicts
    // also, we should listen for a second, as some node can be already in process. And it is good to wait until server is ready when all nodes starts at the same time
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "DroneCAN node getting node ID started");
    while (canardGetLocalNodeID(get_dronecan_instance()) == CANARD_BROADCAST_NODE_ID)
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

void transfer_received_for_dna(CanardInstance *ins, CanardRxTransfer *transfer)
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