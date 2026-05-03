#include "uavcan.protocol.GetNodeInfo-1.h"

/*
    msg name: uavcan.protocol.GetNodeInfo
    msg ID: 1
*/
bool response_1_getNodeInfo(uint8_t destination_node_id, uint8_t *inout_transfer_id, char *unique_id, uint32_t uptime, NodeHealth *node_health, NodeMode *node_mode)
{
    uint8_t major_sw_version = MAJOR_SW_VERSION;
    uint8_t minor_sw_version = MINOR_SW_VERSION;
    uint8_t major_hw_version = MAJOR_HW_VERSION;
    uint8_t minor_hw_version = MINOR_HW_VERSION;
    char name[] = DEVICE_NAME;
    uint8_t name_len = DEVICE_NAME_LEN;

    // 7 - NodeStatus, 15 - SoftwareVersion, 18 - HardwareVersion, cert length zero, 1 - name length, 80 - name
    uint8_t buffer[7 + 15 + 18 + 1 + 1 + DEVICE_NAME_LEN] = {0};

    // NodeStatus
    uint16_t offset = 0;
    canardEncodeScalar(buffer, offset, 32, &uptime);
    offset += 32;
    canardEncodeScalar(buffer, offset, 2, node_health);
    offset += 2;
    canardEncodeScalar(buffer, offset, 3, node_mode);
    offset = 7 * 8; // align to next byte, should be 56

    // SoftwareVersion
    canardEncodeScalar(buffer, offset, 8, &major_sw_version);
    offset += 8;
    canardEncodeScalar(buffer, offset, 8, &minor_sw_version);
    offset = (7 + 15) * 8; // align to next byte, should be 152

    // HardwareVersion
    canardEncodeScalar(buffer, offset, 8, &major_hw_version);
    offset += 8;
    canardEncodeScalar(buffer, offset, 8, &minor_hw_version);
    offset += 8;
    canardEncodeScalar(buffer, offset, 64, unique_id);
    offset += 64;
    canardEncodeScalar(buffer, offset, 64, &unique_id[8]);
    offset = (7 + 15 + 18 + 1) * 8; // align to next byte, should be 320

    // Name
    canardEncodeScalar(buffer, offset, 8, &name_len);
    offset += 8;
    memcpy(&buffer[offset / 8], name, name_len);

    return dronecan_respond(destination_node_id, inout_transfer_id, UAVCAN_GET_NODE_INFO_SIGNATURE,
                            UAVCAN_GET_NODE_INFO_ID,
                            CANARD_TRANSFER_PRIORITY_LOW,
                            buffer,
                            sizeof(buffer));
}