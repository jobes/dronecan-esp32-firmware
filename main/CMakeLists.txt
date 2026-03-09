idf_component_register(
    SRCS 
        "main.c" 
        "dronecan_node.c"
        "app_tasks.c"
        "helpers/dronecan_communication.c"
        "helpers/dronecan_node_state.c"
        "helpers/dronecan_value_params.c"
        "helpers/esp_can.c"
        "messages/uavcan.protocol.GetNodeInfo-1.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        "driver"
        "esp_timer"
        "libcanard"
        "bmp3_sensor_api"
        "nvs_flash"
        "app_update"
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-include" "${CMAKE_CURRENT_LIST_DIR}/project_config.h")