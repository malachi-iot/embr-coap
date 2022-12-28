cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD "pico_w")

# initialize the SDK directly
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(coap-unit-tests LANGUAGES C CXX)

add_subdirectory(../support support)

set(TESTS_DIR ${ROOT_DIR}/test/unity)

add_subdirectory(${TESTS_DIR} unity)

add_executable(${PROJECT_NAME}
    main.cpp
)

# DEBT: unit-test.h could benefit from something cleaner than this
add_compile_definitions(LWIP_PRESENT=1)

target_compile_definitions(${PROJECT_NAME} PRIVATE FEATURE_MCCOAP_LWIP_TRANSPORT=0)

target_include_directories(${PROJECT_NAME} PRIVATE .)
target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR})

target_link_libraries(${PROJECT_NAME}
    pico_cyw43_arch_lwip_poll
    embr::support estd embr mc-coap mc-coap::unity-tests
    unity)

target_link_libraries(coap-unity-tests-lib
    pico_cyw43_arch_lwip_poll
    embr::support
    )

include(../pico_enable.cmake)
