#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

#include "dronecan_communication.h"

bool beginFirmwareUpdate(uint8_t source_node_id, char *path);
void firmware_file_chunk_received(CanardRxTransfer *transfer);
uint8_t get_firmware_source_node_id();
char *get_firmware_path();

#endif // FIRMWARE_UPDATE_H