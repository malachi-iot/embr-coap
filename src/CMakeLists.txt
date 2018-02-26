cmake_minimum_required(VERSION 3.8)
project(mc-coap-test)

#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 98)

include_directories(../ext/Catch/include)
include_directories(../ext/moducom-memory/src)

add_subdirectory(../ext/moducom-memory/src ../bin)

# using technique from https://coderwall.com/p/y3zzbq/use-cmake-enabled-libraries-in-your-cmake-project
#include(ExternalProject)

#ExternalProject_Add(moducom_memory SOURCE_DIR "../../ext/moducom-memory/src")

#find_package ( moducom_memory )

set(SOURCE_FILES
        main.cpp coap.cpp coap.h tests/main_test.cpp tests/memory_pool.cpp tests/coap.cpp
        coap_transmission.cpp coap_transmission.h tests/coap_lowlevel.cpp coap_subscription.cpp
        coap_subscription.h cbor.cpp tests/coap_send.cpp
        tests/string_test.cpp tests/memory_test.cpp coap_daemon.h
        tests/coap_daemon.cpp mc/pipeline.h tests/pipeline_test.cpp
        mc/pipeline.cpp coap-generator.h coap-generator.cpp coap-encoder.cpp
        coap-encoder.h coap-decoder.cpp coap-decoder.h tests/coap-decoder-test.cpp
        mc/pipeline-encoder.h mc/pipeline-writer.h
        mc/pipeline-reader.h
        mc/pipeline-decoder.h coap-dispatcher.cpp coap-dispatcher.h
        tests/coap-dispatcher-test.cpp tests/test-data.h tests/coap-encoder-test.cpp
        coap-blockwise.h coap-blockwise.cpp coap-token.h coap-token.cpp tests/coap-token-test.cpp
        coap-uint.h mc/experimental.h tests/experimental-test.cpp coap-header.h coap-observable.h
        mc/experimental.cpp coap-uripath-dispatcher.cpp coap-uripath-dispatcher.h
        mc/experimental-factory.h coap-features.h
        coap/context.h)
add_executable(mc-coap-test ${SOURCE_FILES})

target_link_libraries(mc-coap-test moducom_memory)