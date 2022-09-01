cmake_minimum_required(VERSION 3.6)

project(mc-coap)

#set(CMAKE_CXX_STANDARD 11)
# NOTE: Need to make this set externally and well tested somehow
# set(CMAKE_CXX_STANDARD 98)

set(ROOT_DIR ..)
set(ESTDLIB_DIR ${ROOT_DIR}/ext/estdlib/src)
set(EMBR_DIR ${ROOT_DIR}/ext/embr/src)

include_directories(${PROJECT_SOURCE_DIR})

include_directories(${ESTDLIB_DIR})
include_directories(${EMBR_DIR})

set(SOURCE_FILES
        coap.cpp coap.h
        #coap_transmission.h
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

        coap/decoder/context.h
        coap/decoder/decode-and-notify.h
        coap/decoder/events.h
        coap/decoder/netbuf.h
        coap/decoder/option.h
        coap/decoder/simple.h

        coap/decoder/streambuf.h
        coap/decoder/streambuf.hpp

        coap/decoder/subject.hpp
        coap/decoder/subject-core.hpp

        coap/encoder/streambuf.h

        coap/observable.h coap/observable.hpp
        coap/platform.h
        coap/option.h
        coap/option-decoder.cpp

        coap/token.cpp
        coap/token.h

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
        exp/uripathmap.h

        exp/prototype/observer-idea1.h

        platform/arduino/arduino-eth-datapump.cpp

        platform/generic/datapump-messageobserver.hpp

        platform/lwip/lwip-datapump.h

        platform/mbedtls/dtls-datapump.cpp
        platform/mbedtls/dtls-datapump.h

        platform/posix/sockets-datapump.h
        platform/posix/sockets-datapump.cpp

        mc/opts.h
        mc/opts-internal.h
        mc/encoder-base.h
        mc/experimental.h
        coap-observable.h

        )

add_library(${PROJECT_NAME} ${SOURCE_FILES})
