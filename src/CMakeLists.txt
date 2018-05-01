cmake_minimum_required(VERSION 3.8)
project(mc-coap)

#set(CMAKE_CXX_STANDARD 11)
# NOTE: Need to make this set externally and well tested somehow
# set(CMAKE_CXX_STANDARD 98)

set(MC_MEM_DIR ../ext/moducom-memory/src)
set(ESTDLIB_DIR ../ext/estdlib/src)

include_directories(${MC_MEM_DIR})
include_directories(${ESTDLIB_DIR})

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../../bin")
# add_subdirectory(. CMAKE_RUNTIME_OUTPUT_DIRECTORY)
# add_subdirectory(${MC_MEM_DIR} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# add_subdirectory(${MC_MEM_DIR} mcmem)
# add_subdirectory(${ESTDLIB_DIR} estdlib)

# using technique from https://coderwall.com/p/y3zzbq/use-cmake-enabled-libraries-in-your-cmake-project
#include(ExternalProject)

#ExternalProject_Add(moducom_memory SOURCE_DIR "../../ext/moducom-memory/src")

#find_package ( moducom_memory )

set(SOURCE_FILES
        coap.cpp coap.h
        coap_transmission.cpp coap_transmission.h coap_subscription.cpp
        coap_subscription.h
        cbor.cpp cbor/features.h cbor/encoder.h
        coap_daemon.h
        exp/datapump.h exp/datapump.hpp
        mc/pipeline.h
        mc/pipeline.cpp coap-generator.h coap-generator.cpp coap-encoder.cpp
        coap-encoder.h coap-decoder.cpp coap-decoder.h
        mc/pipeline-encoder.h mc/pipeline-writer.h
        mc/pipeline-reader.h
        mc/pipeline-decoder.h coap-dispatcher.cpp coap-dispatcher.h
        coap-blockwise.h coap-blockwise.cpp coap-token.h coap-token.cpp
        coap-uint.h mc/experimental.h coap-header.h coap-observable.h
        coap/experimental.cpp coap/experimental-observer.h
        mc/experimental.cpp coap-uripath-dispatcher.cpp coap-uripath-dispatcher.h
        mc/experimental-factory.h coap-features.h
        coap/encoder.h coap/encoder.hpp
        coap/observable.cpp coap/observable.h
        coap/context.h coap/decoder-subject.hpp
        coap/decoder-subject.h coap/option-decoder.cpp)

add_library(moducom-coap-lib ${SOURCE_FILES})
