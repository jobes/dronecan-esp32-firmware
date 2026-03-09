#include "helpers/dronecan_communication.h"
#include "helpers/dronecan_value_params.h"
#include "messages/uavcan.protocol.param.ExecuteOpcode-10.h"
#include "canard.h"
#include "esp_log.h"

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