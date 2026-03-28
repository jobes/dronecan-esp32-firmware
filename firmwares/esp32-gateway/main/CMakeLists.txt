idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS
        "."
        "${PROJECT_DIR}/../../components"
    REQUIRES
        "dronecan_mini"
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-include" "${CMAKE_CURRENT_LIST_DIR}/project_config.h")