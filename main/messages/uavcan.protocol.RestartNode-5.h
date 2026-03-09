#ifndef UAVCAN_PROTOCOL_RESTARTNODE_5_H
#define UAVCAN_PROTOCOL_RESTARTNODE_5_H

#include "helpers/dronecan_communication.h"

#define UAVCAN_RESTART_NODE_ID 5
#define UAVCAN_RESTART_NODE_SIGNATURE 0x569E05394A3017F0ULL
#define UAVCAN_RESTART_NODE_REQUEST_MAGIC_NUMBER 0xACCE551B1EULL

bool check_response_5_restart_transfer_valid(const CanardRxTransfer *transfer);
bool response_5_restartNode(uint8_t destination_node_id, uint8_t *inout_transfer_id);

#endif // UAVCAN_PROTOCOL_RESTARTNODE_5_H
