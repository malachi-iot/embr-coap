cmake_minimum_required(VERSION 2.8)

project(datapump)

set(CMAKE_CXX_STANDARD 11)

# strongly consider making this a features.h phenomena
add_definitions(-DFEATURE_MCCOAP_SOCKETS)

set(MC_COAP_ROOT_DIR ../../..)

set(MC_MEM_DIR ${MC_COAP_ROOT_DIR}/ext/moducom-memory/src)
set(ESTDLIB_DIR ${MC_COAP_ROOT_DIR}/ext/estdlib/src)
set(MC_COAP_DIR ${MC_COAP_ROOT_DIR}/src)

include_directories(${MC_MEM_DIR})
include_directories(${MC_COAP_DIR})
include_directories(${ESTDLIB_DIR})

add_subdirectory(${MC_MEM_DIR} mem)
add_subdirectory(${MC_COAP_DIR} coap)

add_executable(${PROJECT_NAME} "main.cpp")


