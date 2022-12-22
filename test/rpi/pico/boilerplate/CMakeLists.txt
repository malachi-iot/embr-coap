cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD "pico_w")

# initialize the SDK directly
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
include(wifi.cmake)

project(boilerplate LANGUAGES C CXX)

add_subdirectory(../support support)

set(SRC_DIR ${ROOT_DIR}/test/rtos/esp32/boilerplate-subject/main)

add_executable(${PROJECT_NAME}
    main.cpp
    ${SRC_DIR}/app.cpp
    ${TEMPLATE_DIR}/lwip.cpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )

target_compile_definitions(${PROJECT_NAME} PRIVATE FEATURE_MCCOAP_LWIP_TRANSPORT=0)

target_include_directories(${PROJECT_NAME} PRIVATE .)
target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR})

target_link_libraries(${PROJECT_NAME}
    pico_cyw43_arch_lwip_poll
    support estd embr mc-coap)

include(../pico_enable.cmake)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
