cmake_minimum_required(VERSION 3.6)
project(dispatch_simple_server)

set(CMAKE_CXX_STANDARD 11)

# FIX: If we set this to actual __CPP11__, we get compilation problems
#ADD_DEFINITIONS(-D__CPP11___)

set(MC_ROOT ../../..)
set(MEM_LIB_PATH ${MC_ROOT}/ext/moducom-memory/src)
set(COAP_LIB_PATH ${MC_ROOT}/src)

include_directories(${MEM_LIB_PATH})

# due to memory.h & string.h naming collisions
include_directories(${COAP_LIB_PATH})

add_subdirectory(${COAP_LIB_PATH} CMAKE_RUNTIME_OUTPUT_DIRECTORY)

set(SOURCE_FILES main.cpp coap.cpp experimental.h)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} moducom-coap-lib moducom_memory_lib)
