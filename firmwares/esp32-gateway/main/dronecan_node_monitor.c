#include "dronecan_node_monitor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "messages/uavcan.protocol.NodeStatus-341.h"
#include "messages/uavcan.protocol.GetNodeInfo-1.h"
#include "dronecan_mini/dronecan_communication.h"
#include "dronecan_dna_server.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "NODE_MON";

static dronecan_node_t s_nodes[MAX_NODES];
static int s_node_count = 0;

void dronecan_node_monitor_init(void)
{
    memset(s_nodes, 0, sizeof(s_nodes));
    s_node_count = 0;
    ESP_LOGI(TAG, "Node monitor initialized");
}

static dronecan_node_t *find_node(uint8_t node_id)
{
    for (int i = 0; i < s_node_count; i++)
    {
        if (s_nodes[i].node_id == node_id)
        {
            return &s_nodes[i];
        }
    }
    return NULL;
}

// TODO move to messages, use correct hardcoded definitions instead of fe 16
static void request_node_info(uint8_t node_id)
{
    ESP_LOGI(TAG, "New node discovered: %d. Requesting info...", node_id);

    // We don't have a dedicated request function in dronecan_communication.h that is easy to use
    // but we can access the instance directly.
    static uint8_t s_tid = 0;
    uint8_t tid = s_tid++;

    // GetNodeInfo request has empty payload
    canardRequestOrRespond(get_dronecan_instance(), node_id, UAVCAN_GET_NODE_INFO_SIGNATURE, UAVCAN_GET_NODE_INFO_ID, &tid, 16, CanardRequest, NULL, 0);
}

void dronecan_node_monitor_process_transfer(CanardInstance *ins, CanardRxTransfer *transfer)
{
    if (transfer->data_type_id == UAVCAN_NODE_STATUS_ID)
    {
        if (transfer->payload_len < 7)
            return;

        uint32_t uptime_sec = 0;
        canardDecodeScalar(transfer, 0, 28, false, &uptime_sec);

        uint32_t health = 0;
        canardDecodeScalar(transfer, 28, 2, false, &health);

        uint32_t mode = 0;
        canardDecodeScalar(transfer, 30, 3, false, &mode);

        dronecan_node_t *node = find_node(transfer->source_node_id);
        if (!node)
        {
            if (s_node_count < MAX_NODES)
            {
                node = &s_nodes[s_node_count++];
                node->node_id = transfer->source_node_id;
                snprintf(node->name, sizeof(node->name), "Node %d", node->node_id);
                request_node_info(node->node_id);
            }
        }

        if (node)
        {
            node->uptime_sec = uptime_sec;
            node->health = (uint8_t)health;
            node->mode = (uint8_t)mode;
            node->last_seen_ms = (uint32_t)(esp_timer_get_time() / 1000);

            // Try to get UID from DNA server if we don't have it
            bool all_zeros = true;
            for (int i = 0; i < 16; i++)
                if (node->uid[i] != 0)
                {
                    all_zeros = false;
                    break;
                }
            if (all_zeros)
            {
                dronecan_dna_server_get_uid(node->node_id, node->uid);
            }
        }
    }
    else if (transfer->data_type_id == UAVCAN_GET_NODE_INFO_ID && transfer->transfer_type == CanardTransferTypeResponse)
    {
        dronecan_node_t *node = find_node(transfer->source_node_id);
        if (node && transfer->payload_len >= 24 + 16)
        {
            for (int i = 0; i < 16; i++)
            {
                uint64_t val;
                canardDecodeScalar(transfer, (24 + i) * 8, 8, false, &val);
                node->uid[i] = (uint8_t)val;
            }

            // HardwareVersion has certificate_of_authenticity [<=255]
            // It starts with a 1-byte length field (for v0)
            uint64_t cert_len = 0;
            canardDecodeScalar(transfer, 40 * 8, 8, false, &cert_len);

            // Name follows the certificate_of_authenticity
            // certificate_of_authenticity is at offset 41 and has cert_len bytes
            // Name [<=80] starts with a 1-byte length field
            int name_len_offset = 41 + (int)cert_len;
            uint64_t name_len = 0;
            if (transfer->payload_len > name_len_offset)
            {
                canardDecodeScalar(transfer, name_len_offset * 8, 8, false, &name_len);

                int name_data_offset = name_len_offset + 1;
                if (name_len > 0 && transfer->payload_len >= name_data_offset + (int)name_len)
                {
                    if (name_len > sizeof(node->name) - 1)
                        name_len = sizeof(node->name) - 1;
                    for (int i = 0; i < (int)name_len; i++)
                    {
                        uint64_t val;
                        canardDecodeScalar(transfer, (name_data_offset + i) * 8, 8, false, &val);
                        node->name[i] = (char)val;
                    }
                    node->name[name_len] = '\0';
                }
            }

            node->has_info = true;
            ESP_LOGI(TAG, "Got info for node %d: %s", node->node_id, node->name);
        }
    }
}

int dronecan_node_monitor_get_json(char *buf, size_t max_len)
{
    int pos = snprintf(buf, max_len, "[");
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

    for (int i = 0; i < s_node_count; i++)
    {
        dronecan_node_t *n = &s_nodes[i];

        char uid_str[33] = "";
        for (int j = 0; j < 16; j++)
        {
            sprintf(uid_str + j * 2, "%02X", n->uid[j]);
        }

        pos += snprintf(buf + pos, max_len - pos,
                        "%s{\"id\":%d,\"uid\":\"%s\",\"name\":\"%s\",\"uptime\":%u,\"health\":%d,\"mode\":%d,\"last_seen_ms\":%u}",
                        (i == 0) ? "" : ",",
                        n->node_id, uid_str, n->name, (unsigned int)n->uptime_sec, n->health, n->mode, (unsigned int)(now - n->last_seen_ms));

        if (pos >= max_len - 1)
            break;
    }

    pos += snprintf(buf + pos, max_len - pos, "]");
    return pos;
}

void dronecan_node_monitor_tick(void)
{
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    static uint32_t s_last_tick = 0;

    if (now - s_last_tick < 5000)
        return; // Every 5 seconds
    s_last_tick = now;

    for (int i = 0; i < s_node_count; i++)
    {
        dronecan_node_t *n = &s_nodes[i];
        if (!n->has_info && (now - n->last_seen_ms < 10000))
        {
            request_node_info(n->node_id);
        }
    }
}
