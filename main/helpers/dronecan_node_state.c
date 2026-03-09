#include "dronecan_node_state.h"

static NodeHealth node_health = HEALTH_OK;
static NodeMode node_mode = MODE_INITIALIZATION;

void set_node_health(NodeHealth new_health)
{
    node_health = new_health;
}

void set_node_mode(NodeMode new_mode)
{
    node_mode = new_mode;
}

NodeHealth *get_node_health()
{
    return &node_health;
}

NodeMode *get_node_mode()
{
    return &node_mode;
}
