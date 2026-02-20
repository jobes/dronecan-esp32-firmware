#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

// node info
#define DEVICE_NAME "com.inskycore.dronecan_node.pressure_sensor"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define MINOR_SW_VERSION 1
#define MAJOR_SW_VERSION 0

#define MINOR_HW_VERSION 1
#define MAJOR_HW_VERSION 0

// dronecan
#define CAN_TX_PIN GPIO_NUM_21
#define CAN_RX_PIN GPIO_NUM_20
#define CAN_SPEED TWAI_TIMING_CONFIG_1MBITS
#define CAN_CONFIG TWAI_FILTER_CONFIG_ACCEPT_ALL
#define CAN_MODE TWAI_MODE_NORMAL
#define CAN_READ_TIMEOUT_MS 10
#define DRONECAN_MEM_POOL_SIZE 4096
#define DRONECAN_HEARTBEAT_INTERVAL_MS 950 // 1 second interval with some margin

// BMP390
#define BMP_MISO GPIO_NUM_5
#define BMP_MOSI GPIO_NUM_6
#define BMP_SCLK GPIO_NUM_4
#define BMP_CS GPIO_NUM_7
#define SPI_SPEED_HZ 1000000

#endif