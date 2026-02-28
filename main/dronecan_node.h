#ifndef DRONECAN_NODE_H
#define DRONECAN_NODE_H

#include "freertos/FreeRTOS.h"
#include <stdint.h>
#include <stdbool.h>
#include "canard.h"

#define DRONECAN_NODE_ID 42

typedef enum
{
    HEALTH_OK = 0,
    HEALTH_WARNING = 1,
    HEALTH_ERROR = 2,
    HEALTH_CRITICAL = 3
} NodeHealth;

typedef enum
{
    MODE_OPERATIONAL = 0,
    MODE_INITIALIZATION = 1,
    MODE_MAINTENANCE = 2,
    MODE_SOFTWARE_UPDATE = 3,
    MODE_OFFLINE = 7
} NodeMode;

void dronecan_spin(void);
void dronecan_init();

void set_node_health(uint8_t new_health);
void set_node_mode(uint8_t new_mode);
uint8_t *get_node_health(void);
uint8_t *get_node_mode(void);
void get_node_id(void);

bool dronecan_broadcast(uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len, uint8_t *transfer_id);
bool dronecan_respond(uint8_t destination_node_id, uint8_t *inout_transfer_id, uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len);
bool dronecan_request(uint8_t destination_node_id, uint8_t *inout_transfer_id, uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len);

#endif
