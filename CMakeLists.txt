cmake_minimum_required(VERSION 3.16.0)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(fallblatt-code)

list(APPEND EXTRA_COMPONENT_DIRS espressif__mdns)
