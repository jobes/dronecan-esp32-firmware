#ifndef DRONECAN_NODE_MONITOR_H
#define DRONECAN_NODE_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "canard.h"

#define MAX_NODES 128

typedef struct
{
    uint8_t node_id;
    uint8_t uid[16];
    char name[80];
    uint32_t uptime_sec;
    uint8_t health;
    uint8_t mode;
    uint32_t last_seen_ms;
    bool has_info;
} dronecan_node_t;

void dronecan_node_monitor_init(void);
void dronecan_node_monitor_process_transfer(CanardInstance *ins, CanardRxTransfer *transfer);

// Returns JSON representation of nodes
int dronecan_node_monitor_get_json(char *buf, size_t max_len);
void dronecan_node_monitor_tick(void);

#endif // DRONECAN_NODE_MONITOR_H
