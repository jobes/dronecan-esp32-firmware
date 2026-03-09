#ifndef DRONECAN_ACCEPTER_H
#define DRONECAN_ACCEPTER_H

#include "canard.h"

bool should_accept_transfer(
    const CanardInstance *ins,
    uint64_t *out_data_type_signature,
    uint16_t data_type_id,
    CanardTransferType transfer_type,
    uint8_t source_node_id);

#endif // DRONECAN_ACCEPTER_H