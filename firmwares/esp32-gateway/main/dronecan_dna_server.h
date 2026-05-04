#ifndef DRONECAN_DNA_SERVER_H
#define DRONECAN_DNA_SERVER_H

#include "canard.h"

/**
 * @brief Initializes the DroneCAN DNA server.
 * Loads existing allocations from NVS.
 */
void dronecan_dna_server_init(void);

/**
 * @brief Handles incoming transfers for the DNA server.
 * Should be called from the DroneCAN reception callback.
 */
void dronecan_dna_server_handle_transfer(CanardInstance *ins, CanardRxTransfer *transfer);

/**
 * @brief Checks if a transfer should be accepted by the DNA server.
 * Should be called from the DroneCAN should-accept callback.
 */
bool dronecan_dna_server_should_accept(uint16_t data_type_id, CanardTransferType transfer_type);

#endif // DRONECAN_DNA_SERVER_H
