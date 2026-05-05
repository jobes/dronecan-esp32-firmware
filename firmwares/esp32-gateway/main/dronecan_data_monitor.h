#ifndef DRONECAN_DATA_MONITOR_H
#define DRONECAN_DATA_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "canard.h"

#define MAX_DATA_MONITORS 32

typedef struct
{
    uint16_t data_type_id;
    uint64_t data_type_signature;
    uint8_t payload[64];
    uint16_t payload_len;
    uint32_t last_update_ms;
    uint32_t subscribed_until_ms;
} dronecan_data_monitor_t;

void dronecan_data_monitor_init(void);
bool dronecan_data_monitor_should_accept(uint16_t data_type_id, uint64_t *out_data_type_signature);
void dronecan_data_monitor_process_transfer(CanardInstance *ins, CanardRxTransfer *transfer);
void dronecan_data_monitor_update_subscriptions(uint16_t *requested_ids, uint64_t *requested_signatures, int count);
int dronecan_data_monitor_get_json(char *buf, size_t max_len, uint16_t *requested_ids, int count);

#endif // DRONECAN_DATA_MONITOR_H
