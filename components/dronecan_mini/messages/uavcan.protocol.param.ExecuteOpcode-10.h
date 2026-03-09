#ifndef UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_H
#define UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_H

#include "dronecan_communication.h"

#define UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_ID 10
#define UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_SIGNATURE 0x3B131AC5EB69D2CDULL

enum OpcodeAction
{
    OPCODE_SAVE = 0,
    OPCODE_ERASE = 1
};

enum OpcodeAction decode_10_paramExecuteOpcode_process(CanardRxTransfer *transfer);
bool response_10_paramExecuteOpcode_process(CanardRxTransfer *transfer, bool result);

#endif // UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_H