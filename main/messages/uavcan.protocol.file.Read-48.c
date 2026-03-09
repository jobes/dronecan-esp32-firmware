#include "uavcan.protocol.file.Read-48.h"

bool request_read_48(uint8_t destination_node_id, uint64_t offset, const char *path)
{
    static uint8_t transfer_id = 0;
    size_t path_len = strlen(path);
    uint8_t buffer[path_len + 5];
    canardEncodeScalar(buffer, 0, 40, &offset);

    for (size_t i = 0; i < path_len; i++)
    {
        canardEncodeScalar(buffer, 40 + i * 8, 8, &path[i]);
    }

    return dronecan_request(
        destination_node_id,
        &transfer_id,
        UAVCAN_FILE_READ_SIGNATURE,
        UAVCAN_FILE_READ_ID,
        CANARD_TRANSFER_PRIORITY_MEDIUM,
        buffer,
        5 + path_len);
}

bool arrived_response_read_48(const CanardRxTransfer *transfer, char *out_data)
{
    uint16_t error;
    canardDecodeScalar(transfer, 0, 16, false, &error);
    if (error != 0)
    {
        ESP_LOGE("READ_FILE", "Error in read file response: %d", error);
        return false;
    }
    for (size_t i = 0; i < transfer->payload_len - 2; i++)
    {
        canardDecodeScalar(transfer, 16 + i * 8, 8, false, &out_data[i]);
    }

    return true;
}