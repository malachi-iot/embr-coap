cmake_minimum_required(VERSION 2.8)

project(cbor)

set(CMAKE_CXX_STANDARD 11)

set(MC_COAP_ROOT_DIR ../../..)

set(MC_MEM_DIR ${MC_COAP_ROOT_DIR}/ext/moducom-memory/src)
set(ESTDLIB_DIR ${MC_COAP_ROOT_DIR}/ext/estdlib/src)
set(MC_COAP_DIR ${MC_COAP_ROOT_DIR}/src)

include_directories(${PROJECT_SOURCE_DIR})

include_directories(${MC_MEM_DIR})
include_directories(${MC_COAP_DIR})
include_directories(${ESTDLIB_DIR})

add_subdirectory(${MC_MEM_DIR} mem)
add_subdirectory(${MC_COAP_DIR} coap)

add_executable(${PROJECT_NAME}
    "main.cpp"
    mc/opts.h
    simple-cbor-responder.hpp
    )

target_link_libraries(${PROJECT_NAME} moducom-coap-lib moducom_memory_lib)


