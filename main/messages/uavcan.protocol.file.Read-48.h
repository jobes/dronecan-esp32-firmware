#ifndef UAVCAN_PROTOCOL_FILE_READ_48_H
#define UAVCAN_PROTOCOL_FILE_READ_48_H

#include "helpers/dronecan_communication.h"

#define UAVCAN_FILE_READ_ID 48
#define UAVCAN_FILE_READ_SIGNATURE 0x8dcdca939f33f678ULL

bool request_read_48(uint8_t destination_node_id, uint64_t offset, const char *path);
bool arrived_response_read_48(const CanardRxTransfer *transfer, char *out_data);

#endif // UAVCAN_PROTOCOL_FILE_READ_48_H