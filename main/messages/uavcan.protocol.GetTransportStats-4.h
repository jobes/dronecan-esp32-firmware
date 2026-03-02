#ifndef UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_4_H
#define UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_4_H

#include "dronecan_node.h"

#define UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID 4
#define UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_SIGNATURE 0xbe6f76a7ec312b04ULL

static uint64_t logical_tx = 0;
static uint64_t logical_rx = 0;
static uint64_t logical_error = 0;
static uint64_t physical_tx = 0;
static uint64_t physical_rx = 0;

void increase_logical_tx()
{
    logical_tx++;
}
void increase_logical_rx()
{
    logical_rx++;
}
void increase_logical_error()
{
    logical_error++;
}
void increase_physical_tx()
{
    physical_tx++;
}
void increase_physical_rx()
{
    physical_rx++;
}

void response_4_getTransportStats(uint8_t destination_node_id, uint8_t *inout_transfer_id)
{
    twai_status_info_t status;
    twai_get_status_info(&status);
    uint64_t twai_error_count = (uint64_t)status.rx_missed_count +
                                (uint64_t)status.rx_overrun_count +
                                (uint64_t)status.tx_failed_count +
                                (uint64_t)status.arb_lost_count +
                                (uint64_t)status.bus_error_count;

    uint8_t buffer[6 * 6] = {0}; // 48bits (6bytes) per field, 6 fields
    canardEncodeScalar(buffer, 0, 48, &logical_tx);
    canardEncodeScalar(buffer, 48, 48, &logical_rx);
    canardEncodeScalar(buffer, 96, 48, &logical_error);
    canardEncodeScalar(buffer, 144, 48, &physical_tx);
    canardEncodeScalar(buffer, 192, 48, &physical_rx);
    canardEncodeScalar(buffer, 240, 48, &twai_error_count);

    dronecan_respond(destination_node_id,
                     inout_transfer_id,
                     UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_SIGNATURE,
                     UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID,
                     CANARD_TRANSFER_PRIORITY_LOW,
                     buffer,
                     sizeof(buffer));
}

#endif // UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_4_H