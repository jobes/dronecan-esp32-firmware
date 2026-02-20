idf_component_register(
    SRCS 
        "main.c" 
        "dronecan_node.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        "driver"
        "esp_timer"
        "libcanard"
        "bmp3_sensor_api"
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-include" "${CMAKE_CURRENT_LIST_DIR}/project_config.h")