#include "messages/uavcan.protocol.GetTransportStats-4.h"

bool response_4_getTransportStats(uint8_t destination_node_id, uint8_t *inout_transfer_id, uint64_t phys_er, uint64_t phys_tx, uint64_t phys_rx, uint64_t log_er, uint64_t log_tx, uint64_t log_rx)
{
    uint8_t buffer[6 * 6] = {0}; // 48bits (6bytes) per field, 6 fields

    canardEncodeScalar(buffer, 0, 48, &log_tx);
    canardEncodeScalar(buffer, 48, 48, &log_rx);
    canardEncodeScalar(buffer, 96, 48, &log_er);
    canardEncodeScalar(buffer, 144, 48, &phys_tx);
    canardEncodeScalar(buffer, 192, 48, &phys_rx);
    canardEncodeScalar(buffer, 240, 48, &phys_er);

    return dronecan_respond(destination_node_id,
                            inout_transfer_id,
                            UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_SIGNATURE,
                            UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID,
                            CANARD_TRANSFER_PRIORITY_LOW,
                            buffer,
                            sizeof(buffer));
}