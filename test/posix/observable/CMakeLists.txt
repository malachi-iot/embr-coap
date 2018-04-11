cmake_minimum_required(VERSION 3.8)

project(observable-test)

set(CMAKE_CXX_STANDARD 11)

set(MC_ROOT ../../..)
set(MC_MEM_DIR ${MC_ROOT}/ext/moducom-memory/src)
set(MC_COAP_DIR ${MC_ROOT}/src)

add_subdirectory(${MC_COAP_DIR} CMAKE_RUNTIME_OUTPUT_DIRECTORY)

include_directories(${MC_MEM_DIR})
include_directories(${MC_COAP_DIR})

set(SOURCE_FILES
        observable.cpp
	main.cpp)

add_executable(observable-test ${SOURCE_FILES})

target_link_libraries(observable-test moducom-coap-lib moducom_memory_lib)
