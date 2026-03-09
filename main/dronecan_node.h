#ifndef DRONECAN_NODE_H
#define DRONECAN_NODE_H

#include "freertos/FreeRTOS.h"
#include <stdint.h>
#include <stdbool.h>
#include "canard.h"

#define DRONECAN_NODE_ID 42

void dronecan_spin(void);

void get_node_id(void);
void on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer);
bool should_accept_transfer(const CanardInstance *ins, uint64_t *out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id);

#endif
