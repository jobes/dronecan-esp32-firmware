#ifndef DRONECAN_NODE_H
#define DRONECAN_NODE_H

#include <stdint.h>
#include <stdbool.h>
#include "canard.h"

#define DRONECAN_NODE_ID 42

#define UAVCAN_NODE_STATUS_ID 341
#define UAVCAN_NODE_STATUS_SIGNATURE 0x0F0868D0C1A7C6F1ULL

#define UAVCAN_EQUIPMENT_AIR_DATA_STATICPRESSURE_ID 1028
#define UAVCAN_EQUIPMENT_AIR_DATA_STATICPRESSURE_SIGNATURE 0x44DC4133A6B487BAULL

#define UAVCAN_GET_NODE_INFO_ID 1
#define UAVCAN_GET_NODE_INFO_SIGNATURE 0xEE468A8121C46A9EULL

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

void set_node_health(uint8_t new_health);
void set_node_mode(uint8_t new_mode);
void dronecan_init();
void dronecan_spin(void);
void dronecan_publish_node_status(void);
bool dronecan_publish_static_pressure(_Float32 pressure_pa, _Float16 variance_pa2);
bool dronecan_broadcast(uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len, uint8_t *transfer_id);

#endif
