set(LIBCANARD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../submodules/libcanard")
idf_component_register(
    SRCS 
        "${LIBCANARD_DIR}/canard.c"
    INCLUDE_DIRS 
        "${LIBCANARD_DIR}"
)