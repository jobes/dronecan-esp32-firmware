#include "uavcan.protocol.NodeStatus-341.h"

bool publish_341_nodeStatus(uint32_t uptime, NodeHealth *health, NodeMode *mode)
{
    static uint8_t transfer_id = 0;
    uint8_t buffer[7] = {0};

    canardEncodeScalar(buffer, 0, 32, &uptime);
    canardEncodeScalar(buffer, 32, 2, health);
    canardEncodeScalar(buffer, 34, 3, mode);

    return dronecan_broadcast(UAVCAN_NODE_STATUS_SIGNATURE,
                              UAVCAN_NODE_STATUS_ID,
                              CANARD_TRANSFER_PRIORITY_LOW,
                              buffer,
                              sizeof(buffer),
                              &transfer_id);
}