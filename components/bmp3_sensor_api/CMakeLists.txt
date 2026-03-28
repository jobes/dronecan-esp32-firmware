set(BMP3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../submodules/bmp3_sensor_api")
idf_component_register(
    SRCS 
        "${BMP3_DIR}/bmp3.c"
        "bmp390.c"
    INCLUDE_DIRS 
        "${BMP3_DIR}"
        "."
    REQUIRES 
        "driver"
        "esp_driver_spi"
        "esp_driver_gpio"
)

if(EXISTS "${MY_GLOBAL_CONFIG}")
    target_compile_options(${COMPONENT_LIB} PRIVATE "-include" "${MY_GLOBAL_CONFIG}")
endif()