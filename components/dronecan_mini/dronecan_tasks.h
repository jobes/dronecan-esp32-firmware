#ifndef DRONECAN_TASKS_H
#define DRONECAN_TASKS_H

#include "canard.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef void (*dronecan_frame_listener_t)(const CanardCANFrame *frame);
typedef dronecan_frame_listener_t dronecan_rx_frame_listener_t;


void init_tasks(TaskFunction_t app_task, uint8_t node_id, BaseType_t core_id);
void dronecan_set_rx_frame_listener(dronecan_frame_listener_t listener);
void dronecan_set_tx_frame_listener(dronecan_frame_listener_t listener);
void dronecan_inject_rx_frame(const CanardCANFrame *frame);

#endif // DRONECAN_TASKS_H