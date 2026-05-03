#include "uavcan.protocol.GetTransportStats-4.h"

bool response_4_getTransportStats(uint8_t destination_node_id, uint8_t *inout_transfer_id, uint64_t log_er, uint64_t log_tx, uint64_t log_rx, uavcan_protocol_GetTransportStats_CanIfaceStats *iface_stats, uint8_t ifaces_len)
{
    if (ifaces_len > 3)
        ifaces_len = 3;
    uint8_t buffer[18 + 18 * 3] = {0}; // 18 bytes for logical stats, up to 3 * 18 for ifaces

    canardEncodeScalar(buffer, 0, 48, &log_tx);
    canardEncodeScalar(buffer, 48, 48, &log_rx);
    canardEncodeScalar(buffer, 96, 48, &log_er);

    int bit_offset = 144;
    for (int i = 0; i < ifaces_len; i++)
    {
        canardEncodeScalar(buffer, bit_offset, 48, &iface_stats[i].tx_requests);
        canardEncodeScalar(buffer, bit_offset + 48, 48, &iface_stats[i].rx_messages);
        canardEncodeScalar(buffer, bit_offset + 96, 48, &iface_stats[i].errors);
        bit_offset += 144;
    }

    return dronecan_respond(destination_node_id,
                            inout_transfer_id,
                            UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_SIGNATURE,
                            UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID,
                            CANARD_TRANSFER_PRIORITY_LOW,
                            buffer,
                            bit_offset / 8);
}