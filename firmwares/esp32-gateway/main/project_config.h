#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

// node info
#define DEVICE_NAME "com.inskycore.dronecan_node.gateway"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define MINOR_SW_VERSION 1
#define MAJOR_SW_VERSION 0

#define MINOR_HW_VERSION 1
#define MAJOR_HW_VERSION 0

// dronecan
#define CAN_TX_PIN GPIO_NUM_20
#define CAN_RX_PIN GPIO_NUM_10
#define CAN_SPEED TWAI_TIMING_CONFIG_1MBITS
#define CAN_CONFIG TWAI_FILTER_CONFIG_ACCEPT_ALL
#define CAN_MODE TWAI_MODE_NORMAL
#define CAN_READ_TIMEOUT_MS 10
#define DRONECAN_MEM_POOL_SIZE 4096
#define DRONECAN_HEARTBEAT_INTERVAL_MS 950 // 1 second interval with some margin

#endif