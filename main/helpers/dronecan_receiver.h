#ifndef DRONECAN_RECEIVER_H
#define DRONECAN_RECEIVER_H

#include "canard.h"

void on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer);
uint64_t get_logical_rx();

#endif // DRONECAN_RECEIVER_H