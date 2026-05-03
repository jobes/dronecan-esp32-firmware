#ifndef UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_4_H
#define UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_4_H

#include "dronecan_communication.h"

#define UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID 4
#define UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_SIGNATURE 0xbe6f76a7ec312b04ULL

typedef struct
{
    uint64_t tx_requests;
    uint64_t rx_messages;
    uint64_t errors;
} uavcan_protocol_GetTransportStats_CanIfaceStats;

bool response_4_getTransportStats(uint8_t destination_node_id, uint8_t *inout_transfer_id, uint64_t log_er, uint64_t log_tx, uint64_t log_rx, uavcan_protocol_GetTransportStats_CanIfaceStats *iface_stats, uint8_t ifaces_len);

#endif // UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_4_H