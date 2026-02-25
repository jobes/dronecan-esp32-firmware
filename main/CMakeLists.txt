idf_component_register(
    SRCS 
        "main.c" 
        "dronecan_node.c"
        "app_tasks.c"
        "helpers/dronecan_value_params.c"
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