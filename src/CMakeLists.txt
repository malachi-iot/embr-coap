cmake_minimum_required(VERSION 3.6)

project(mc-coap)

#set(CMAKE_CXX_STANDARD 11)
# NOTE: Need to make this set externally and well tested somehow
# set(CMAKE_CXX_STANDARD 98)

set(ROOT_DIR ..)
set(MC_MEM_DIR ${ROOT_DIR}/ext/moducom-memory/src)
set(ESTDLIB_DIR ${ROOT_DIR}/ext/estdlib/src)
set(EMBR_DIR ${ROOT_DIR}/ext/embr/src)

include_directories(${PROJECT_SOURCE_DIR})

include_directories(${MC_MEM_DIR})
include_directories(${ESTDLIB_DIR})
include_directories(${EMBR_DIR})

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
        coap_transmission.cpp coap_transmission.h
        cbor.h
        cbor/cbor.cpp cbor/features.h
        cbor/decoder.cpp
        cbor/decoder.h
        cbor/encoder.h

        coap/blockwise.h

        coap/context.h

        coap/decoder.cpp
        coap/decoder.h
        coap/decoder.hpp

        coap/decoder/events.h
        coap/decoder/netbuf.h
        coap/decoder/observer.h
        coap/decoder/observer-aggregate.h
        coap/decoder/observer-aggregate.hpp
        coap/decoder/option.h
        coap/decoder/simple.h

        coap/decoder/streambuf.h
        coap/decoder/streambuf.hpp

        coap/decoder/subject.h
        coap/decoder/subject.hpp

        coap/encoder.h coap/encoder.hpp
        coap/encoder/streambuf.h

        coap/experimental-observer.h
        coap/observable.h coap/observable.hpp
        coap/platform.h
        coap/option.h
        coap/option-decoder.cpp

        coap/token.cpp
        coap/token.h

        coap-dispatcher.cpp coap-dispatcher.hpp coap-dispatcher.h
        coap-encoder.h
        coap-encoder.cpp
        coap-features.h
        coap-uripath-dispatcher.hpp coap-uripath-dispatcher.h

        exp/datapump-observer.h
        exp/events.h
        exp/factory.h
        exp/message-observer.h
        exp/misc.h
        exp/misc.hpp
        exp/option-trait.h
        exp/retry.h
        exp/diagnostic-decoder-observer.h
        exp/uripath-repeater.h

        exp/prototype/observer-idea1.h

        platform/arduino/arduino-eth-datapump.cpp

        platform/generic/datapump-messageobserver.hpp
        platform/generic/malloc_netbuf.h

        platform/lwip/lwip-datapump.h
        platform/lwip/lwip-portandaddress.h

        platform/mbedtls/dtls-datapump.cpp
        platform/mbedtls/dtls-datapump.h

        platform/posix/sockets-datapump.h
        platform/posix/sockets-datapump.cpp

        mc/opts.h
        mc/opts-internal.h
        mc/encoder-base.h
        #mc/pipeline.h
        #mc/pipeline.cpp
        #mc/pipeline-encoder.h mc/pipeline-writer.h
        #mc/pipeline-reader.h
        #mc/pipeline-decoder.h
        mc/experimental.h
        coap-observable.h

        mc/experimental.cpp
        mc/experimental-factory.h
        )

# DEBT: get rid of old moducom-coap-lib extended name
add_library(moducom-coap-lib ${SOURCE_FILES})
add_library(${PROJECT_NAME} ALIAS moducom-coap-lib)
