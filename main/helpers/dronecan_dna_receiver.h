#ifndef DRONECAN_DNA_RECEIVER_H
#define DRONECAN_DNA_RECEIVER_H

#include "canard.h"

void get_node_id();
void transfer_received_for_dna(CanardInstance *ins, CanardRxTransfer *transfer);
bool should_accept_transfer_for_dna(const CanardInstance *ins, uint64_t *out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id);
#endif // DRONECAN_DNA_RECEIVER_H