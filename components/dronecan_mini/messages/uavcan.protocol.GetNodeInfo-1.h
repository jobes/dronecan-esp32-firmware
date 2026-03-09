#ifndef UAVCAN_PROTOCOL_GETNODEINFO_1_H
#define UAVCAN_PROTOCOL_GETNODEINFO_1_H

#include "dronecan_communication.h"
#include "dronecan_node_state.h"

#define UAVCAN_GET_NODE_INFO_ID 1
#define UAVCAN_GET_NODE_INFO_SIGNATURE 0xEE468A8121C46A9EULL

/*
    msg name: uavcan.protocol.GetNodeInfo
    msg ID: 1
*/
bool response_1_getNodeInfo(uint8_t destination_node_id, uint8_t *inout_transfer_id, char *unique_id, uint32_t uptime, NodeHealth *node_health, NodeMode *node_mode);

#endif // UAVCAN_PROTOCOL_GETNODEINFO_1_H