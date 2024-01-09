cmake_minimum_required(VERSION 3.13)

# initialize the SDK directly
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(embr-coap-support LANGUAGES C CXX)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../..)
get_filename_component(ROOT_DIR ${ROOT_DIR} ABSOLUTE)

add_subdirectory(${ROOT_DIR}/ext/embr/test/rpi/pico/support support)
add_subdirectory(${ROOT_DIR}/src coap)

# Fun fact: seems INTERFACE libs can export these scopes
# without a consuming add_library on the other side
set(ROOT_DIR ${ROOT_DIR} PARENT_SCOPE)
set(TEMPLATE_DIR ${ROOT_DIR}/test/template PARENT_SCOPE)

add_library(${PROJECT_NAME} INTERFACE)

add_library(support ALIAS ${PROJECT_NAME})
add_library(embr::coap::support ALIAS ${PROJECT_NAME})
