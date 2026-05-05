idf_component_register(
    SRCS
        "main.c"
        "wifi_manager.c"
        "http_server.c"
        "cannelloni_bridge.c"
        "dronecan_dna_server.c"
        "dronecan_node_monitor.c"
        "dronecan_data_monitor.c"
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
        "web/template.html"
        "web/status.html"
        "web/system.html"
        "web/ap.html"
        "web/sta.html"
        "web/nodes.html"
        "web/style.css"
        "web/nodes.css"
        "web/common.js"
        "web/nodes.js"
        "web/status.js"
        "web/monitor.html"
        "web/monitor.css"
        "web/monitor.js"
    EMBED_FILES
        "web/favicon.ico"
)



target_compile_options(${COMPONENT_LIB} PRIVATE "-include" "${CMAKE_CURRENT_LIST_DIR}/project_config.h")