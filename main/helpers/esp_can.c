#include "esp_can.h"
#include "dronecan_communication.h"

static const char *TAG = "ESP_CAN";

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