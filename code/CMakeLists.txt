cmake_minimum_required(VERSION 3.16)

set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(drone_remote_id_scanner)

if(NOT IDF_TARGET STREQUAL "esp32c61")
    message(FATAL_ERROR "This project uses ESP32-C61. Run: idf.py set-target esp32c61")
endif()
if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}" VERSION_LESS "5.5.5")
    message(FATAL_ERROR "This app uses ESP-IDF 5.5.5 or newer. Activate the C61 tool installation.")
endif()
