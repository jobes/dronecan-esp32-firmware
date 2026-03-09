#ifndef UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_1_H
#define UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_1_H

#include "helpers/dronecan_communication.h"
#include "canard.h"
#include <string.h>

#define UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID 1
#define UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE 0xb2a812620a11d40ULL

enum AllocationMsg
{
    FIRST_PART = 0,
    SECOND_PART = 1,
    FINAL_PART = 2
};
static enum AllocationMsg lastSentPart = FIRST_PART;

uint8_t process_1_dynamicNodeIdAllocation(CanardRxTransfer *transfer)
{
    uint8_t node_id = 0;
    for (int i = 0; i < 16; i++)
    {
        uint8_t unique_id_char;
        canardDecodeScalar(transfer, i * 8 + 8, 8, false, &unique_id_char);
        if (unique_id_char != get_unique_id()[i])
        {
            canardDecodeScalar(transfer, 0, 7, false, &node_id);
            return 0; // unique ID does not match, ignore this allocation message
        }
    }
    canardDecodeScalar(transfer, 0, 7, false, &node_id);

    return node_id;
}

bool publish_1_dynamicNodeIdAllocation(uint8_t preferred_node_id, enum AllocationMsg msg_part, char *uniques_id)
{
    static uint8_t transfer_id = 0;
    uint8_t buffer[7] = {0};
    lastSentPart = msg_part;
    if (msg_part == FIRST_PART)
    {
        buffer[0] = 1;
    }
    canardEncodeScalar(buffer, 0, 7, &preferred_node_id);
    // 6 characters of unique ID, 16 total; first msg = 6; second msg = 6; third msg = 4
    uint8_t msg_part_size = (msg_part == FINAL_PART) ? 4 : 6;
    for (int i = 0; i < msg_part_size; i++)
    {
        canardEncodeScalar(buffer, 8 + i * 8, 8, &uniques_id[msg_part * 6 + i]);
    }

    return dronecan_broadcast(UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE,
                              UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID,
                              CANARD_TRANSFER_PRIORITY_HIGH,
                              buffer, msg_part_size + 1, &transfer_id);
}

#endif // UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_1_H
