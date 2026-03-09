#ifndef UAVCAN_PROTOCOL_NODESTATUS_341_H
#define UAVCAN_PROTOCOL_NODESTATUS_341_H

#include "dronecan_communication.h"
#include "dronecan_node_state.h"

#define UAVCAN_NODE_STATUS_ID 341
#define UAVCAN_NODE_STATUS_SIGNATURE 0x0F0868D0C1A7C6F1ULL

/*
    msg name: uavcan.protocol.NodeStatus
    msg ID: 341
*/
bool publish_341_nodeStatus(uint32_t uptime, NodeHealth *health, NodeMode *mode);

#endif // UAVCAN_PROTOCOL_NODESTATUS_341_H