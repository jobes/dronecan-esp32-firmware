#ifndef UAVCAN_PROTOCOL_NODESTATUS_341_H
#define UAVCAN_PROTOCOL_NODESTATUS_341_H

#include "helpers/dronecan_communication.h"
#include "helpers/dronecan_node_state.h"

#define UAVCAN_NODE_STATUS_ID 341
#define UAVCAN_NODE_STATUS_SIGNATURE 0x0F0868D0C1A7C6F1ULL

/*
    msg name: uavcan.protocol.NodeStatus
    msg ID: 341
*/
bool publish_341_nodeStatus()
{
    static uint8_t transfer_id = 0;
    uint8_t buffer[7] = {0};

    uint32_t uptime = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;

    canardEncodeScalar(buffer, 0, 32, &uptime);
    canardEncodeScalar(buffer, 32, 2, get_node_health());
    canardEncodeScalar(buffer, 34, 3, get_node_mode());

    return dronecan_broadcast(UAVCAN_NODE_STATUS_SIGNATURE,
                              UAVCAN_NODE_STATUS_ID,
                              CANARD_TRANSFER_PRIORITY_LOW,
                              buffer,
                              sizeof(buffer),
                              &transfer_id);
}

#endif // UAVCAN_PROTOCOL_NODESTATUS_341_H
