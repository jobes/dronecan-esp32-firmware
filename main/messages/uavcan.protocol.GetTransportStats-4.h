#ifndef UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_4_H
#define UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_4_H

#include "helpers/dronecan_communication.h"

#define UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID 4
#define UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_SIGNATURE 0xbe6f76a7ec312b04ULL

bool response_4_getTransportStats(uint8_t destination_node_id, uint8_t *inout_transfer_id, uint64_t phys_er, uint64_t phys_tx, uint64_t phys_rx, uint64_t log_er, uint64_t log_tx, uint64_t log_rx);

#endif // UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_4_H