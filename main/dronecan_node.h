#ifndef DRONECAN_NODE_H
#define DRONECAN_NODE_H

#include "freertos/FreeRTOS.h"
#include <stdint.h>
#include <stdbool.h>
#include "canard.h"

bool should_accept_transfer(const CanardInstance *ins, uint64_t *out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id);

#endif
