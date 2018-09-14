cmake_minimum_required(VERSION 3.6)

project(mc-coap-test)

#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

add_definitions(-DUNIT_TESTING)
#add_definitions(-DDEBUG)

set(ROOT_DIR ../..)
set(MC_COAP_DIR ${ROOT_DIR}/src)

include_directories(${MC_COAP_DIR})
add_subdirectory(${MC_COAP_DIR} coap)

include_directories(${PROJECT_SOURCE_DIR})

#message(${PROJECT_SOURCE_DIR})

set(MC_MEM_DIR ${ROOT_DIR}/ext/moducom-memory/src)
set(ESTDLIB_DIR ${ROOT_DIR}/ext/estdlib/src)
set(EMBR_DIR ${ROOT_DIR}/ext/embr/src)

include_directories(${ROOT_DIR}/ext/Catch/include)

include_directories(${MC_MEM_DIR})
include_directories(${ESTDLIB_DIR})

include_directories(${EMBR_DIR})

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../../bin")
# add_subdirectory(. CMAKE_RUNTIME_OUTPUT_DIRECTORY)
# add_subdirectory(${MC_MEM_DIR} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# doesn't work because MC_MEM is already pulling it in this way, but
# preferably MC_MEM would only do include_directories.  Refactor MC_MEM
# for that
# add_subdirectory(${ESTDLIB_DIR} estd)

add_subdirectory(${MC_MEM_DIR} mem)

add_subdirectory(${EMBR_DIR} embr)

# using technique from https://coderwall.com/p/y3zzbq/use-cmake-enabled-libraries-in-your-cmake-project
#include(ExternalProject)

#ExternalProject_Add(moducom_memory SOURCE_DIR "../../ext/moducom-memory/src")

#find_package ( moducom_memory )

set(SOURCE_FILES
        main_test.cpp coap.cpp
        coap_lowlevel.cpp
        cbor-test.cpp
        coap_daemon.cpp
        coap-datapump-test.cpp
        coap-decoder-test.cpp
        coap-dispatcher-test.cpp test-data.h coap-encoder-test.cpp
        coap-token-test.cpp
        coap-observable-test.cpp
        blockwise-test.cpp
        dataport-test.cpp
        experimental-test.cpp
        decoder-subject-test.cpp
        mc/opts.h
        netbuf-test.cpp
        test-observer.h experimental2-test.cpp
        retry-test.cpp
        uint-test.cpp
        )

add_executable(mc-coap-test ${SOURCE_FILES})

target_link_libraries(mc-coap-test moducom-coap-lib moducom_memory_lib embr-lib)

