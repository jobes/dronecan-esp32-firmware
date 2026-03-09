idf_component_register(
    SRCS
        "dronecan_tasks.c"
        "dronecan_communication.c"
        "dronecan_node_state.c"
        "dronecan_value_params.c"
        "esp_can.c"
        "dronecan_dna_receiver.c"
        "dronecan_receiver.c"
        "dronecan_accepter.c"
        "firmware_update.c"
        "messages/uavcan.protocol.GetNodeInfo-1.c"
        "messages/uavcan.protocol.dynamic_node_id.Allocation-1.c"
        "messages/uavcan.protocol.GetTransportStats-4.c"
        "messages/uavcan.protocol.file.BeginFirmwareUpdate-40.c"
        "messages/uavcan.protocol.file.Read-48.c"
        "messages/uavcan.protocol.param.ExecuteOpcode-10.c"
        "messages/uavcan.protocol.param.GetSet-11.c"
        "messages/uavcan.protocol.RestartNode-5.c"
        "messages/uavcan.equipment.air_data.StaticPressure-1028.c"
        "messages/uavcan.equipment.air_data.StaticTemperature-1029.c"
        "messages/uavcan.protocol.NodeStatus-341.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "driver"
        "esp_timer"
        "libcanard"
        "nvs_flash"
        "app_update"
)

# Keep legacy config macros available to moved dronecan sources.
target_compile_options(${COMPONENT_LIB} PRIVATE "-include" "${PROJECT_DIR}/main/project_config.h")
