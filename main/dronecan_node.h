#ifndef DRONECAN_NODE_H
#define DRONECAN_NODE_H

#include <stdint.h>
#include <stdbool.h>
#include "canard.h"

#define DRONECAN_NODE_ID 42
#define DRONECAN_MEM_POOL_SIZE 4096
#define CAN_BITRATE 1000000

#define UAVCAN_NODE_STATUS_ID 341
#define UAVCAN_NODE_STATUS_SIGNATURE 0x0F0868D0C1A7C6F1ULL

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

void dronecan_init();
void dronecan_spin(void);
void dronecan_publish_node_status(void);
bool dronecan_broadcast(uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len);
static volatile uint32_t g_uptime_sec = 0; // TODO remove global variable and use xTaskGetTickCount() * portTICK_PERIOD_MS * 1000 instead

#endif
