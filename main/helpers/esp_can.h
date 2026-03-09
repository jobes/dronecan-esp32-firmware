#ifndef ESP_CAN_H
#define ESP_CAN_H

#include "esp_log.h"
#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

bool can_driver_init(void);

#endif // ESP_CAN_H