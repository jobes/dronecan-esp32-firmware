#ifndef ESP_CAN_H
#define ESP_CAN_H

#include "esp_log.h"
#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "canard.h"

bool can_driver_init(void);
bool can_transmit(const CanardCANFrame *frame);
bool can_receive(CanardCANFrame *frame);

uint64_t get_physical_tx();
uint64_t get_physical_rx();

#endif // ESP_CAN_H