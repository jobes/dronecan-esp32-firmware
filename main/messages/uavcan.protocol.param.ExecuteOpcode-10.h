#ifndef UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_H
#define UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_H

#include "dronecan_node.h"

#define UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_ID 10
#define UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_SIGNATURE 0x3B131AC5EB69D2CDULL

bool response_10_paramExecuteOpcode_process(CanardRxTransfer *transfer)
{
    uint8_t opcode = 0;
    canardDecodeScalar(transfer, 0, 8, false, &opcode);
    bool ok = false;
    if (opcode == 0)
    {

        ESP_LOGI("OPCODE", "Saving parameters to NVS");
        ok = save_parameters_to_nvs();
        ESP_LOGI("OPCODE", "Saving parameters to NVS result: %s", ok ? "OK" : "FAIL");
    }
    else if (opcode == 1)
    { // Erase
        ESP_LOGI("OPCODE", "Erasing parameters from NVS");
        ok = erase_parameters_from_nvs();
        ESP_LOGI("OPCODE", "Erasing parameters from NVS result: %s", ok ? "OK" : "FAIL");
    }

    uint8_t buffer[7] = {0};
    canardEncodeScalar(buffer, 48, 1, &ok);

    return dronecan_respond(transfer->source_node_id,
                            &transfer->transfer_id,
                            UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_SIGNATURE,
                            UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_ID,
                            CANARD_TRANSFER_PRIORITY_LOW,
                            buffer,
                            sizeof(buffer));
}

#endif // UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_H