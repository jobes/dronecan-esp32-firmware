cmake_minimum_required(VERSION 3.16)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "${REPO_ROOT}/components")
set(MY_GLOBAL_CONFIG "${CMAKE_CURRENT_LIST_DIR}/main/project_config.h" CACHE INTERNAL "")
project(esp32_airpressure)