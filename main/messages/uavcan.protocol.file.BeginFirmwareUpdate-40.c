#include "esp_log.h"
#include "helpers/dronecan_communication.h"
#include "messages/uavcan.protocol.file.Read-48.h"
#include "esp_ota_ops.h"
#include "canard.h"
#include "helpers/dronecan_node_state.h"

#include "messages/uavcan.protocol.file.BeginFirmwareUpdate-40.h"

void process_40_beginFirmwareUpdate(CanardRxTransfer *transfer, uint8_t *firmware_source_node_id, char **firmware_path)
{

    canardDecodeScalar(transfer, 0, 8, false, firmware_source_node_id);
    *firmware_path = malloc(transfer->payload_len);
    (*firmware_path)[transfer->payload_len - 1] = '\0';
    for (int i = 0; i < transfer->payload_len - 1; i++)
    {
        canardDecodeScalar(transfer, (i + 1) * 8, 8, false, &(*firmware_path)[i]);
    }
}

bool response_40_beginFirmwareUpdate(enum RESPONSE_TYPE response_type, uint8_t destination_node_id, uint8_t *transfer_id)
{
    uint8_t buffer[2] = {0};
    buffer[0] = (uint8_t)response_type;

    return dronecan_respond(
        destination_node_id,
        transfer_id,
        UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_SIGNATURE,
        UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_ID,
        CANARD_TRANSFER_PRIORITY_HIGH,
        buffer,
        sizeof(buffer));
}
