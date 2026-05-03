#include "dronecan_node_state.h"
#include "esp_mac.h"
#include "esp_log.h"

static NodeHealth node_health = HEALTH_OK;
static NodeMode node_mode = MODE_INITIALIZATION;
static char UNIQUE_ID[17] = "INSKYCORE_";

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

void init_unique_id()
{
    esp_read_mac((uint8_t *)&UNIQUE_ID[10], ESP_MAC_WIFI_STA);
    UNIQUE_ID[16] = '\0';
    ESP_LOGI("GET_NODE", "Unique ID: %s", UNIQUE_ID);
    ESP_LOG_BUFFER_HEX("GET_NODE", UNIQUE_ID, 16);
}

char *get_unique_id()
{
    return UNIQUE_ID;
}
