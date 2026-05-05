#include "dronecan_data_monitor.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#define SUBSCRIPTION_TIMEOUT_MS 10000

static dronecan_data_monitor_t s_monitors[MAX_DATA_MONITORS];

void dronecan_data_monitor_init(void)
{
    memset(s_monitors, 0, sizeof(s_monitors));
}

bool dronecan_data_monitor_should_accept(uint16_t data_type_id, uint64_t *out_data_type_signature)
{
    if (out_data_type_signature == NULL)
    {
        return false;
    }

    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    for (int i = 0; i < MAX_DATA_MONITORS; i++)
    {
        if (s_monitors[i].subscribed_until_ms > now &&
            s_monitors[i].data_type_id == data_type_id)
        {
            *out_data_type_signature = s_monitors[i].data_type_signature;
            return true;
        }
    }
    return false;
}

void dronecan_data_monitor_process_transfer(CanardInstance *ins, CanardRxTransfer *transfer)
{
    if (transfer->transfer_type != CanardTransferTypeBroadcast)
        return;

    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

    for (int i = 0; i < MAX_DATA_MONITORS; i++)
    {
        if (s_monitors[i].subscribed_until_ms > now &&
            s_monitors[i].data_type_id == transfer->data_type_id)
        {

            size_t len = transfer->payload_len;
            if (len > 64)
                len = 64;

            for (size_t k = 0; k < len; k++)
            {
                uint64_t val;
                canardDecodeScalar(transfer, k * 8, 8, false, &val);
                s_monitors[i].payload[k] = (uint8_t)val;
            }

            s_monitors[i].payload_len = (uint16_t)len;
            s_monitors[i].last_update_ms = now;
            break;
        }
    }
}

void dronecan_data_monitor_update_subscriptions(uint16_t *requested_ids, uint64_t *requested_signatures, int count)
{
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

    for (int i = 0; i < count; i++)
    {
        uint16_t id = requested_ids[i];
        uint64_t signature = requested_signatures[i];
        if (signature == 0)
        {
            continue;
        }

        int existing = -1;
        int free_slot = -1;

        for (int j = 0; j < MAX_DATA_MONITORS; j++)
        {
            if (s_monitors[j].subscribed_until_ms > now &&
                s_monitors[j].data_type_id == id)
            {
                existing = j;
                break;
            }
            if (free_slot == -1 && s_monitors[j].subscribed_until_ms <= now)
            {
                free_slot = j;
            }
        }

        if (existing != -1)
        {
            s_monitors[existing].data_type_signature = signature;
            s_monitors[existing].subscribed_until_ms = now + SUBSCRIPTION_TIMEOUT_MS;
        }
        else if (free_slot != -1)
        {
            memset(&s_monitors[free_slot], 0, sizeof(dronecan_data_monitor_t));
            s_monitors[free_slot].data_type_id = id;
            s_monitors[free_slot].data_type_signature = signature;
            s_monitors[free_slot].subscribed_until_ms = now + SUBSCRIPTION_TIMEOUT_MS;
        }
    }
}

int dronecan_data_monitor_get_json(char *buf, size_t max_len, uint16_t *requested_ids, int count)
{
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

    // Build JSON response
    int pos = snprintf(buf, max_len, "{");

    for (int i = 0; i < count; i++)
    {
        uint16_t id = requested_ids[i];
        int found_idx = -1;
        for (int j = 0; j < MAX_DATA_MONITORS; j++)
        {
            if (s_monitors[j].subscribed_until_ms > now &&
                s_monitors[j].data_type_id == id)
            {
                found_idx = j;
                break;
            }
        }

        if (found_idx != -1 && s_monitors[found_idx].last_update_ms != 0)
        {
            pos += snprintf(buf + pos, max_len - pos, "\"%" PRIu16 "\":{\"payload\":\"", id);
            for (int k = 0; k < s_monitors[found_idx].payload_len; k++)
            {
                pos += snprintf(buf + pos, max_len - pos, "%02X", s_monitors[found_idx].payload[k]);
            }
            pos += snprintf(buf + pos, max_len - pos, "\",\"age_ms\":%" PRIu32 "}", now - s_monitors[found_idx].last_update_ms);
        }
        else
        {
            pos += snprintf(buf + pos, max_len - pos, "\"%" PRIu16 "\":null", id);
        }

        if (i < count - 1)
        {
            pos += snprintf(buf + pos, max_len - pos, ",");
        }

        if (pos >= (int)max_len - 10)
            break;
    }

    pos += snprintf(buf + pos, max_len - pos, "}");
    return pos;
}
