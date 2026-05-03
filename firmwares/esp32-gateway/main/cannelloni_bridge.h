#ifndef CANNELLONI_BRIDGE_H
#define CANNELLONI_BRIDGE_H

#include "esp_log.h"
#include "canard.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start the Cannelloni to DroneCAN bridge task.
 * 
 * This function creates a UDP socket on the configured port and starts
 * tasks for bidirectional CAN frame bridging.
 */
void cannelloni_bridge_init(void);

/**
 * @brief Push a CAN frame to be sent over Cannelloni UDP.
 * 
 * @param frame The CAN frame to bridge.
 */
void cannelloni_bridge_push_frame(const CanardCANFrame *frame);

#ifdef __cplusplus
}
#endif

#endif // CANNELLONI_BRIDGE_H
