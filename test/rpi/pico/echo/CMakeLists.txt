cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD "pico_w")

# initialize the SDK directly
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
include(wifi.cmake)

project(echo LANGUAGES C CXX)

add_subdirectory(../support support)

set(SRC_DIR ${ROOT_DIR}/test/rtos/esp32/echo/main)

add_executable(${PROJECT_NAME}
    main.cpp
    ${SRC_DIR}/udp-coap.cpp
    ${TEMPLATE_DIR}/lwip.cpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )

target_include_directories(${PROJECT_NAME} PRIVATE .)

target_link_libraries(${PROJECT_NAME}
    pico_cyw43_arch_lwip_poll
    embr::support estd embr mc-coap)

include(../pico_enable.cmake)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
