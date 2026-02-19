#include "dronecan_node.h"
#include "canard.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char* TAG = "DroneCAN";

// Libcanard inštancia a pamäťový pool
static CanardInstance g_canard;
static uint8_t g_canard_memory_pool[DRONECAN_MEM_POOL_SIZE];

// Synchronizácia
static SemaphoreHandle_t g_canard_mutex;

// Uptime counter
static uint32_t g_uptime_sec = 0;

// ============================================================================
// CAN Driver (TWAI) pre ESP32-C3
// ============================================================================

static bool can_driver_init(void) {
    // TWAI konfigurácia - GPIO piny pre ESP32-C3
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        GPIO_NUM_4,  // TX pin
        GPIO_NUM_5,  // RX pin
        TWAI_MODE_NORMAL
    );

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Inštalácia TWAI drivera
    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TWAI driver");
        return false;
    }

    // Štart TWAI drivera
    if (twai_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start TWAI driver");
        return false;
    }

    ESP_LOGI(TAG, "CAN driver initialized");
    return true;
}

static bool can_transmit(const CanardCANFrame* frame) {
    twai_message_t message = {0};

    message.identifier = frame->id & CANARD_CAN_EXT_ID_MASK;
    message.extd = 1;  // DroneCAN používa extended ID
    message.data_length_code = frame->data_len;
    memcpy(message.data, frame->data, frame->data_len);

    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(10));
    return (result == ESP_OK);
}

static bool can_receive(CanardCANFrame* frame) {
    twai_message_t message;

    if (twai_receive(&message, pdMS_TO_TICKS(1)) != ESP_OK) {
        return false;
    }

    frame->id = message.identifier | CANARD_CAN_FRAME_EFF;
    frame->data_len = message.data_length_code;
    memcpy(frame->data, message.data, message.data_length_code);

    return true;
}

// ============================================================================
// Libcanard Callback funkcie
// ============================================================================

static bool should_accept_transfer(
    const CanardInstance* ins,
    uint64_t* out_data_type_signature,
    uint16_t data_type_id,
    CanardTransferType transfer_type,
    uint8_t source_node_id
) {
    (void)ins;
    (void)source_node_id;

    // Akceptujeme NodeStatus broadcast
    if (transfer_type == CanardTransferTypeBroadcast) {
        if (data_type_id == UAVCAN_NODE_STATUS_ID) {
            *out_data_type_signature = UAVCAN_NODE_STATUS_SIGNATURE;
            return true;
        }
    }

    // Akceptujeme GetNodeInfo request
    if (transfer_type == CanardTransferTypeRequest) {
        if (data_type_id == UAVCAN_GET_NODE_INFO_ID) {
            *out_data_type_signature = UAVCAN_GET_NODE_INFO_SIGNATURE;
            return true;
        }
    }

    return false;
}

static void on_transfer_received(CanardInstance* ins, CanardRxTransfer* transfer) {
    ESP_LOGI(TAG, "Transfer received: type_id=%d, from_node=%d",
             transfer->data_type_id, transfer->source_node_id);

    // Spracovanie GetNodeInfo requestu
    if (transfer->transfer_type == CanardTransferTypeRequest &&
        transfer->data_type_id == UAVCAN_GET_NODE_INFO_ID) {

        // Odpoveď s informáciami o node
        uint8_t response[64] = {0};

    // NodeStatus časť (7 bajtov)
    response[0] = (uint8_t)(g_uptime_sec);
    response[1] = (uint8_t)(g_uptime_sec >> 8);
    response[2] = (uint8_t)(g_uptime_sec >> 16);
    response[3] = (uint8_t)(g_uptime_sec >> 24);
    response[4] = (HEALTH_OK << 6) | MODE_OPERATIONAL;
    response[5] = 0;  // vendor specific status
    response[6] = 0;

    // Software version
    response[7] = 1;  // major
    response[8] = 0;  // minor

    // Hardware version
    response[9] = 1;   // major
    response[10] = 0;  // minor

    // Unique ID (16 bytes) - použijeme MAC adresu
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    memcpy(&response[11], mac, 6);

    // Name
    const char* name = "ESP32C3_DroneCAN";
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

// ============================================================================
// DroneCAN Public API
// ============================================================================

void dronecan_init(void) {
    ESP_LOGI(TAG, "Initializing DroneCAN node...");

    // Vytvorenie mutexu
    g_canard_mutex = xSemaphoreCreateMutex();
    configASSERT(g_canard_mutex != NULL);

    // Inicializácia CAN drivera
    if (!can_driver_init()) {
        ESP_LOGE(TAG, "CAN driver init failed!");
        return;
    }

    // Inicializácia Libcanard
    canardInit(&g_canard,
               g_canard_memory_pool,
               sizeof(g_canard_memory_pool),
               on_transfer_received,
               should_accept_transfer,
               NULL);

    canardSetLocalNodeID(&g_canard, DRONECAN_NODE_ID);

    ESP_LOGI(TAG, "DroneCAN initialized with Node ID: %d", DRONECAN_NODE_ID);
}

void dronecan_publish_node_status(void) {
    // NodeStatus správa (7 bajtov)
    uint8_t buffer[7];

    // Uptime v sekundách (4 bajty, little endian)
    buffer[0] = (uint8_t)(g_uptime_sec);
    buffer[1] = (uint8_t)(g_uptime_sec >> 8);
    buffer[2] = (uint8_t)(g_uptime_sec >> 16);
    buffer[3] = (uint8_t)(g_uptime_sec >> 24);

    // Health (2 bity) + Mode (3 bity) + Sub-mode (3 bity)
    buffer[4] = (HEALTH_OK << 6) | (MODE_OPERATIONAL << 3);

    // Vendor specific status code (2 bajty)
    buffer[5] = 0;
    buffer[6] = 0;

    dronecan_broadcast(UAVCAN_NODE_STATUS_SIGNATURE,
                       UAVCAN_NODE_STATUS_ID,
                       CANARD_TRANSFER_PRIORITY_LOW,
                       buffer,
                       sizeof(buffer));
}

bool dronecan_broadcast(uint64_t signature, uint16_t type_id,
                        uint8_t priority, const void* payload, uint16_t len) {

    xSemaphoreTake(g_canard_mutex, portMAX_DELAY);

    static uint8_t transfer_id = 0;

    int16_t result = canardBroadcast(&g_canard,
                                     signature,
                                     type_id,
                                     &transfer_id,
                                     priority,
                                     payload,
                                     len);

    xSemaphoreGive(g_canard_mutex);

    if (result <= 0) {
        ESP_LOGE(TAG, "Broadcast failed: %d", result);
        return false;
    }

    return true;
                        }

                        void dronecan_spin(void) {
                            xSemaphoreTake(g_canard_mutex, portMAX_DELAY);

                            // Odoslanie všetkých čakajúcich framov
                            const CanardCANFrame* frame;
                            while ((frame = canardPeekTxQueue(&g_canard)) != NULL) {
                                if (can_transmit(frame)) {
                                    canardPopTxQueue(&g_canard);
                                } else {
                                    break;  // TX buffer plný, skúsime neskôr
                                }
                            }

                            // Príjem framov
                            CanardCANFrame rx_frame;
                            while (can_receive(&rx_frame)) {
                                canardHandleRxFrame(&g_canard, &rx_frame,
                                                    xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);
                            }

                            // Cleanup starých transferov
                            canardCleanupStaleTransfers(&g_canard,
                                                        xTaskGetTickCount() * portTICK_PERIOD_MS * 1000);

                            xSemaphoreGive(g_canard_mutex);
                        }
