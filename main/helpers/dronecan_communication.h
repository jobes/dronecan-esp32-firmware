#ifndef DRONECAN_COMMUNICATION_H
#define DRONECAN_COMMUNICATION_H

#include "canard.h"
#include "freertos/FreeRTOS.h"

void dronecan_init(CanardOnTransferReception on_reception, CanardShouldAcceptTransfer should_accept);
bool dronecan_broadcast(uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len, uint8_t *transfer_id);
bool dronecan_respond(uint8_t destination_node_id, uint8_t *inout_transfer_id, uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len);
bool dronecan_request(uint8_t destination_node_id, uint8_t *inout_transfer_id, uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len);

SemaphoreHandle_t get_dronecan_communication_semaphore();
CanardInstance *get_dronecan_instance();
uint64_t get_logical_error();
uint64_t get_logical_tx();
void increase_logical_error();

#endif // DRONECAN_COMMUNICATION_H