#include "uavcan.protocol.param.ExecuteOpcode-10.h"

enum OpcodeAction decode_10_paramExecuteOpcode_process(CanardRxTransfer *transfer)
{
    uint8_t opcode = 0;
    canardDecodeScalar(transfer, 0, 8, false, &opcode);
    if (opcode)
    {
        return OPCODE_ERASE;
    }
    return OPCODE_SAVE;
}

bool response_10_paramExecuteOpcode_process(CanardRxTransfer *transfer, bool result)
{
    uint8_t buffer[7] = {0};
    canardEncodeScalar(buffer, 48, 1, &result);

    return dronecan_respond(transfer->source_node_id,
                            &transfer->transfer_id,
                            UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_SIGNATURE,
                            UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_ID,
                            CANARD_TRANSFER_PRIORITY_LOW,
                            buffer,
                            sizeof(buffer));
}