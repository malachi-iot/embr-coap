cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD "pico_w")

# initialize the SDK directly
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(echo LANGUAGES C CXX)

set(ROOT_DIR ../../../..)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# picks up estd for us also
add_subdirectory(${ROOT_DIR}/ext/embr/src embr)
# DEBT: coap lib doesn't truly include embr/estd - perhaps make it an interface
# library, because it implicitly is already
add_subdirectory(${ROOT_DIR}/src coap)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE .)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    malachi-iot::estd mc-coap)

# enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# wait indefinitely for a CDC to appear so we can see test results
# Beware -- reports indicate this hangs UART indefinitely
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=-1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})
