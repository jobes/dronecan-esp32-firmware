#ifndef DRONECAN_NODE_STATE_H
#define DRONECAN_NODE_STATE_H

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

void set_node_health(NodeHealth new_health);
void set_node_mode(NodeMode new_mode);
NodeHealth *get_node_health(void);
NodeMode *get_node_mode(void);
char *get_unique_id();
void init_unique_id();

#endif