#ifndef UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_1_H
#define UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_1_H

#include "helpers/dronecan_communication.h"

#define UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID 1
#define UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE 0xb2a812620a11d40ULL

enum AllocationMsg
{
    FIRST_PART = 0,
    SECOND_PART = 1,
    FINAL_PART = 2
};

uint8_t process_1_dynamicNodeIdAllocation(CanardRxTransfer *transfer, char *unique_id);
bool publish_1_dynamicNodeIdAllocation(uint8_t preferred_node_id, enum AllocationMsg msg_part, char *unique_id);

#endif // UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_1_H
