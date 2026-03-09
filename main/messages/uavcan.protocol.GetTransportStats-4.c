#include "helpers/dronecan_communication.h"
#include "helpers/esp_can.h"
#include "helpers/dronecan_receiver.h"
#include "messages/uavcan.protocol.GetTransportStats-4.h"

// TODO make get data from params
void response_4_getTransportStats(uint8_t destination_node_id, uint8_t *inout_transfer_id)
{
    twai_status_info_t status;
    twai_get_status_info(&status);
    uint64_t physical_error = (uint64_t)status.rx_missed_count +
                              (uint64_t)status.rx_overrun_count +
                              (uint64_t)status.tx_failed_count +
                              (uint64_t)status.arb_lost_count +
                              (uint64_t)status.bus_error_count;
    uint64_t physical_tx = get_physical_tx();
    uint64_t physical_rx = get_physical_rx();
    uint64_t logical_error = get_logical_error();
    uint64_t logical_tx = get_logical_tx();
    uint64_t logical_rx = get_logical_rx();
    uint8_t buffer[6 * 6] = {0}; // 48bits (6bytes) per field, 6 fields

    canardEncodeScalar(buffer, 0, 48, &logical_tx);
    canardEncodeScalar(buffer, 48, 48, &logical_rx);
    canardEncodeScalar(buffer, 96, 48, &logical_error);
    canardEncodeScalar(buffer, 144, 48, &physical_tx);
    canardEncodeScalar(buffer, 192, 48, &physical_rx);
    canardEncodeScalar(buffer, 240, 48, &physical_error);

    dronecan_respond(destination_node_id,
                     inout_transfer_id,
                     UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_SIGNATURE,
                     UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID,
                     CANARD_TRANSFER_PRIORITY_LOW,
                     buffer,
                     sizeof(buffer));
}