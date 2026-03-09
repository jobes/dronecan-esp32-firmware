#ifndef UAVCAN_PROTOCOL_RESTARTNODE_5_H
#define UAVCAN_PROTOCOL_RESTARTNODE_5_H

#include "helpers/dronecan_communication.h"

#define UAVCAN_RESTART_NODE_ID 5
#define UAVCAN_RESTART_NODE_SIGNATURE 0x569E05394A3017F0ULL
#define UAVCAN_RESTART_NODE_REQUEST_MAGIC_NUMBER 0xACCE551B1EULL

bool check_response_5_restart_transfer_valid(const CanardRxTransfer *transfer)
{
    if (transfer == NULL || transfer->payload_len < 5)
    {
        return false;
    }

    uint64_t magic_number = 0;
    canardDecodeScalar(transfer, 0, 40, false, &magic_number);
    return magic_number == UAVCAN_RESTART_NODE_REQUEST_MAGIC_NUMBER;
}

bool response_5_restartNode(uint8_t destination_node_id, uint8_t *inout_transfer_id)
{
    uint8_t buffer[1] = {0};                         // A single byte indicating that the restart is allowed (1 for allowed, 0 for not allowed)
    canardEncodeScalar(buffer, 0, 1, &(uint8_t){1}); // Set the byte to 1 to indicate that the restart is allowed

    return dronecan_respond(destination_node_id,
                            inout_transfer_id,
                            UAVCAN_RESTART_NODE_SIGNATURE,
                            UAVCAN_RESTART_NODE_ID,
                            CANARD_TRANSFER_PRIORITY_LOW,
                            buffer,
                            sizeof(buffer));
}

#endif // UAVCAN_PROTOCOL_RESTARTNODE_5_H
