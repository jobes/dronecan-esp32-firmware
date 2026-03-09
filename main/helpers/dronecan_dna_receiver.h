#ifndef DRONECAN_DNA_RECEIVER_H
#define DRONECAN_DNA_RECEIVER_H

#include "canard.h"

void get_node_id();
void transfer_received_for_dna(CanardInstance *ins, CanardRxTransfer *transfer);

#endif // DRONECAN_DNA_RECEIVER_H