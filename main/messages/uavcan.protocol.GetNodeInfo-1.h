#ifndef UAVCAN_PROTOCOL_GETNODEINFO_1_H
#define UAVCAN_PROTOCOL_GETNODEINFO_1_H

#include "helpers/dronecan_communication.h"
#include "helpers/dronecan_node_state.h"
#include "esp_mac.h"
#include "esp_log.h"

#define UAVCAN_GET_NODE_INFO_ID 1
#define UAVCAN_GET_NODE_INFO_SIGNATURE 0xEE468A8121C46A9EULL

void init_unique_id();
char *get_unique_id();

/*
    msg name: uavcan.protocol.GetNodeInfo
    msg ID: 1
*/
bool response_1_getNodeInfo(uint8_t destination_node_id, uint8_t *inout_transfer_id);

#endif // UAVCAN_PROTOCOL_GETNODEINFO_1_H