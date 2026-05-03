idf_component_register(
    SRCS
        "main.c"
        "wifi_manager.c"
        "http_server.c"
        "cannelloni_bridge.c"
    INCLUDE_DIRS
        "."
        "${PROJECT_DIR}/../../components"
    REQUIRES
        "dronecan_mini"
        "esp_wifi"
        "esp_netif"
        "nvs_flash"
        "mdns"
        "esp_http_server"
    EMBED_TXTFILES
        "template.html"
        "status.html"
        "system.html"
        "ap.html"
        "sta.html"
)



target_compile_options(${COMPONENT_LIB} PRIVATE "-include" "${CMAKE_CURRENT_LIST_DIR}/project_config.h")