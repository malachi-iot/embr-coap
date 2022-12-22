cmake_minimum_required(VERSION 3.13)

# initialize the SDK directly
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(support LANGUAGES C CXX)

include(../setup.cmake)

set(ROOT_DIR ${ROOT_DIR} PARENT_SCOPE)
set(TEMPLATE_DIR ${TEMPLATE_DIR} PARENT_SCOPE)

set(SOURCE_FILES init.cpp loop.cpp)
add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})

#target_compile_definitions(${PROJECT_NAME} PRIVATE
#        WIFI_SSID=\"${WIFI_SSID}\"
#        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
#        )

# Gets esp_log.h wrapper - might be better as a tiny interface library instead though. hmm
target_include_directories(${PROJECT_NAME} PUBLIC ${ROOT_DIR}/ext/estdlib/src/estd/port/esp-idf/fake)
target_include_directories(${PROJECT_NAME} PUBLIC .)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    estd embr mc-coap
    )

# DEBT: Unsure how this meshes with app also setting this variable
pico_enable_stdio_usb(${PROJECT_NAME} 1)


