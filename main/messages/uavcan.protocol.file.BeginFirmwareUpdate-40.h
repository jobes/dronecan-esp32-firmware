#ifndef UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_40_H
#define UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_40_H

#include "canard.h"

#define UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_ID 40
#define UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_SIGNATURE 0xb7d725df72724126ULL

enum RESPONSE_TYPE
{
    OK = 0,
    INVALID_MODE = 1,
    IN_PROGRESS = 2,
    UNKNOWN = 3
};

bool process_40_beginFirmwareUpdate(CanardRxTransfer *transfer);

bool response_40_beginFirmwareUpdate(enum RESPONSE_TYPE response_type, uint8_t destination_node_id, uint8_t *transfer_id);

void firmware_file_chunk_received(CanardRxTransfer *transfer);

uint8_t get_firmware_source_node_id();
char *get_firmware_path();

#endif // UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_40_H