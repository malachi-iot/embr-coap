cmake_minimum_required(VERSION 3.8)

project(mc-coap-test)

#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 98)

set(MC_MEM_DIR ../../ext/moducom-memory/src)
set(ESTDLIB_DIR ../../ext/estdlib/src)
set(MC_COAP_DIR ..)

include_directories(../../ext/Catch/include)
include_directories(${MC_MEM_DIR})
include_directories(${MC_COAP_DIR})
include_directories(${ESTDLIB_DIR})

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../../bin")
# add_subdirectory(. CMAKE_RUNTIME_OUTPUT_DIRECTORY)
# add_subdirectory(${MC_MEM_DIR} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_subdirectory(${MC_MEM_DIR} mem)
add_subdirectory(${MC_COAP_DIR} coap)

# using technique from https://coderwall.com/p/y3zzbq/use-cmake-enabled-libraries-in-your-cmake-project
#include(ExternalProject)

#ExternalProject_Add(moducom_memory SOURCE_DIR "../../ext/moducom-memory/src")

#find_package ( moducom_memory )

set(SOURCE_FILES
        main_test.cpp coap.cpp
        coap_lowlevel.cpp
        cbor-test.cpp
        coap_send.cpp
        coap_daemon.cpp pipeline_test.cpp
        coap-decoder-test.cpp
        coap-dispatcher-test.cpp test-data.h coap-encoder-test.cpp
        coap-token-test.cpp
        coap-observable-test.cpp
        experimental-test.cpp
        decoder-subject-test.cpp
        test-observer.h experimental2-test.cpp)

add_executable(mc-coap-test ${SOURCE_FILES})

target_link_libraries(mc-coap-test moducom-coap-lib moducom_memory_lib)

