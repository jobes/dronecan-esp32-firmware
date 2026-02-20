cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(MY_GLOBAL_CONFIG "${CMAKE_CURRENT_LIST_DIR}/main/project_config.h" CACHE INTERNAL "")
project(esp32c3_dronecan)