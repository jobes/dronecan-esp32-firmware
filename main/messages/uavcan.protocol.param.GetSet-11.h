#ifndef UAVCAN_PROTOCOL_PARAM_GETSET_11_H
#define UAVCAN_PROTOCOL_PARAM_GETSET_11_H

#include "helpers/dronecan_communication.h"
#include "helpers/dronecan_value_params.h"

#define UAVCAN_PARAM_GETSET_ID 11
#define UAVCAN_PARAM_GETSET_SIGNATURE 0xA7B622F939D1A4D5ULL

bool response_11_paramGetSet(uint8_t destination_node_id, uint8_t *inout_transfer_id, union DeviceParameter *data);
bool response_11_paramGetSetEmpty(uint8_t destination_node_id, uint8_t *inout_transfer_id);
uint16_t decode_11_paramGetSet_request(CanardRxTransfer *transfer, union DeviceParameter *data, uint16_t data_length);

#endif // UAVCAN_PROTOCOL_PARAM_GETSET_11_H
