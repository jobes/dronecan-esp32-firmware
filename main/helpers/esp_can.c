#include "esp_can.h"
#include "dronecan_communication.h"

static const char *TAG = "ESP_CAN";
static uint64_t physical_tx = 0;
static uint64_t physical_rx = 0;

bool can_driver_init()
{
    twai_mode_t mode = TWAI_MODE_NORMAL;
    twai_timing_config_t t_config = CAN_SPEED();
    twai_filter_config_t f_config = CAN_CONFIG();
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, mode);
    g_config.rx_queue_len = 128;

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

bool can_transmit(const CanardCANFrame *frame)
{
    twai_message_t message = {0};

    message.identifier = frame->id & CANARD_CAN_EXT_ID_MASK;
    message.extd = 1;
    message.data_length_code = frame->data_len;
    memcpy(message.data, frame->data, frame->data_len);

    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(10));
    if (result == ESP_OK)
    {
        physical_tx++;
    }
    return (result == ESP_OK);
}

bool can_receive(CanardCANFrame *frame)
{
    twai_message_t message;

    if (twai_receive(&message, pdMS_TO_TICKS(1)) != ESP_OK)
    {
        return false;
    }
    physical_rx++;

    frame->id = message.identifier | CANARD_CAN_FRAME_EFF;
    frame->data_len = message.data_length_code;
    memcpy(frame->data, message.data, message.data_length_code);

    return true;
}

uint64_t get_physical_tx()
{
    return physical_tx;
}

uint64_t get_physical_rx()
{
    return physical_rx;
}