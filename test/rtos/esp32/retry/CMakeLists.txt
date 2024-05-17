cmake_minimum_required(VERSION 3.5)

set(COMPONENTS main)

include(../setenv.cmake)

# 17MAY24 MB - feels close to working, but doesn't
# Reference: https://docs.espressif.com/projects/esp-idf/en/v5.1.4/esp32/api-guides/host-apps.html
#list(APPEND EXTRA_COMPONENT_DIRS"$ENV{IDF_PATH}/tools/mocks/driver/")
#list(APPEND EXTRA_COMPONENT_DIRS"$ENV{IDF_PATH}/tools/mocks/esp_wifi/")
#list(APPEND EXTRA_COMPONENT_DIRS"$ENV{IDF_PATH}/tools/mocks/tcp_transport/")

project(retry)
