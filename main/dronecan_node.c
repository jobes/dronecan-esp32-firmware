#include "dronecan_node.h"
#include "canard.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_mac.h"

static const char *TAG = "DroneCAN";

static CanardInstance g_canard;
static uint8_t g_canard_memory_pool[DRONECAN_MEM_POOL_SIZE];

static SemaphoreHandle_t g_canard_mutex;
static uint8_t node_health = HEALTH_OK;
static uint8_t node_mode = MODE_INITIALIZATION;

static bool can_driver_init()
{
    twai_mode_t mode = CAN_MODE;
    twai_timing_config_t t_config = CAN_SPEED();
    twai_filter_config_t f_config = CAN_CONFIG();
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, mode);

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to install TWAI driver");
        return false;
    }

    if (twai_start() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start TWAI driver");
        return false;
    }

    ESP_LOGI(TAG, "CAN driver initialized");
    return true;
}

static bool can_transmit(const CanardCANFrame *frame)
{
    twai_message_t message = {0};

    message.identifier = frame->id & CANARD_CAN_EXT_ID_MASK;
    message.extd = 1;
    message.data_length_code = frame->data_len;
    memcpy(message.data, frame->data, frame->data_len);

    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(10));
    return (result == ESP_OK);
}

static bool can_receive(CanardCANFrame *frame)
{
    twai_message_t message;

    if (twai_receive(&message, pdMS_TO_TICKS(1)) != ESP_OK)
    {
        return false;
    }

    frame->id = message.identifier | CANARD_CAN_FRAME_EFF;
    frame->data_len = message.data_length_code;
    memcpy(frame->data, message.data, message.data_length_code);

    return true;
}

static bool should_accept_transfer(
    const CanardInstance *ins,
    uint64_t *out_data_type_signature,
    uint16_t data_type_id,
    CanardTransferType transfer_type,
    uint8_t source_node_id)
{
    (void)ins;
    (void)source_node_id;

    if (transfer_type == CanardTransferTypeBroadcast)
    {
        if (data_type_id == UAVCAN_NODE_STATUS_ID)
        {
            *out_data_type_signature = UAVCAN_NODE_STATUS_SIGNATURE;
            return true;
        }
    }

    if (transfer_type == CanardTransferTypeRequest)
    {
        if (data_type_id == UAVCAN_GET_NODE_INFO_ID)
        {
            *out_data_type_signature = UAVCAN_GET_NODE_INFO_SIGNATURE;
            return true;
        }
    }

    return false;
}

static void on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer)
{
    ESP_LOGI(TAG, "Transfer received: type_id=%d, from_node=%d",
             transfer->data_type_id, transfer->source_node_id);

    if (transfer->transfer_type == CanardTransferTypeRequest &&
        transfer->data_type_id == UAVCAN_GET_NODE_INFO_ID)
    {

        uint8_t response[64] = {0};
        uint32_t uptime = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;

        response[0] = uptime;
        response[1] = uptime >> 8;
        response[2] = uptime >> 16;
        response[3] = uptime >> 24;
        response[4] = (node_health << 6) | (node_mode << 3);
        response[5] = 0;
        response[6] = 0;

        response[7] = 1;
        response[8] = 0;

        response[9] = 1;
        response[10] = 0;

        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        memcpy(&response[11], mac, 6);

        const char *name = UNIQUE_NAME;
        size_t name_len = strlen(name);
        response[27] = name_len;
        memcpy(&response[28], name, name_len);

        canardRequestOrRespond(ins,
                               transfer->source_node_id,
                               UAVCAN_GET_NODE_INFO_SIGNATURE,
                               UAVCAN_GET_NODE_INFO_ID,
                               &transfer->transfer_id,
                               CANARD_TRANSFER_PRIORITY_MEDIUM,
                               CanardResponse,
                               response,
                               28 + name_len);
    }
}

void set_node_health(uint8_t new_health)
{
    node_health = new_health;
}

void set_node_mode(uint8_t new_mode)
{
    node_mode = new_mode;
}

void dronecan_init()
{
    ESP_LOGI(TAG, "Initializing DroneCAN node...");

    g_canard_mutex = xSemaphoreCreateMutex();
    configASSERT(g_canard_mutex != NULL);

    if (!can_driver_init())
    {
        ESP_LOGE(TAG, "CAN driver init failed!");
        return;
    }

    canardInit(&g_canard,
               g_canard_memory_pool,
               sizeof(g_canard_memory_pool),
               on_transfer_received,
               should_accept_transfer,
               NULL);

    canardSetLocalNodeID(&g_canard, DRONECAN_NODE_ID); // TODO make it dynamic

    ESP_LOGI(TAG, "DroneCAN initialized with Node ID: %d", DRONECAN_NODE_ID);
}

void dronecan_publish_node_status(void) // TODO check if this is full and correct
{
    static uint8_t transfer_id = 0;
    uint32_t uptime = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
    uint8_t buffer[7];

    buffer[0] = uptime;
    buffer[1] = uptime >> 8;
    buffer[2] = uptime >> 16;
    buffer[3] = uptime >> 24;

    buffer[4] = (node_health << 6) | (node_mode << 3);

    buffer[5] = 0;
    buffer[6] = 0;

    dronecan_broadcast(UAVCAN_NODE_STATUS_SIGNATURE,
                       UAVCAN_NODE_STATUS_ID,
                       CANARD_TRANSFER_PRIORITY_LOW,
                       buffer,
                       sizeof(buffer),
                       &transfer_id);
}

bool dronecan_broadcast(uint64_t signature, uint16_t type_id, uint8_t priority, const void *payload, uint16_t len, uint8_t *transfer_id) // TODO check if this is full and correct
{

    xSemaphoreTake(g_canard_mutex, portMAX_DELAY);

    int16_t result = canardBroadcast(&g_canard,
                                     signature,
                                     type_id,
                                     transfer_id,
                                     priority,
                                     payload,
                                     len);

    xSemaphoreGive(g_canard_mutex);

    if (result <= 0)
    {
        ESP_LOGE(TAG, "Broadcast failed: %d", result);
        return false;
    }

    return true;
}

void dronecan_spin(void)
{
    xSemaphoreTake(g_canard_mutex, portMAX_DELAY);

    const CanardCANFrame *frame;
    while ((frame = canardPeekTxQueue(&g_canard)) != NULL)
    {
        if (can_transmit(frame))
        {
            canardPopTxQueue(&g_canard);
        }
        else
        {
            break;
        }
    }

    CanardCANFrame rx_frame;
    while (can_receive(&rx_frame))
    {
        canardHandleRxFrame(&g_canard, &rx_frame, xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);
    }

    canardCleanupStaleTransfers(&g_canard, xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);

    xSemaphoreGive(g_canard_mutex);
}

// TODO send real values;
// TODO parameter getters and setters;
// TODO restart
// TODO FW updater
// TODO rewrite to a library style, main.c show then have only app task.
// TODO vendor specific code, software version, hw version, cert of authenticity
// TODO dronecan.uavcan.protocol.gettransportstats
// TODO turn off wifi and bluetooth and everything not needed for the node to save power