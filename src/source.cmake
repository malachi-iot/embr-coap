include(${CMAKE_CURRENT_LIST_DIR}/../tools/cmake/setvars.cmake)

set(SOURCE_FILES
        coap.cpp
        coap-encoder.cpp
        cbor/cbor.cpp
        cbor/decoder.cpp
        coap/decoder.cpp
        coap/decoder/observer/util.cpp
        coap/option-decoder.cpp
        coap/token.cpp
        )

# Not directly used, keeping only for posterity
set(INCLUDE_FILES
        coap.h

        cbor.h
        cbor/features.h
        cbor/decoder.h
        cbor/encoder.h

        coap/blockwise.h

        coap/context.h

        coap/decoder.h
        coap/decoder.hpp

        coap/decoder/context.h
        coap/decoder/decode-and-notify.h
        coap/decoder/events.h
        coap/decoder/option.h
        coap/decoder/simple.h

        coap/decoder/streambuf.h
        coap/decoder/streambuf.hpp

        coap/decoder/subject.hpp
        coap/decoder/subject-core.hpp

        coap/encoder/streambuf.h

        coap/platform.h
        coap/option.h

        coap/token.h

        coap-encoder.h
        coap-features.h

        exp/events.h
        exp/factory.h
        exp/message-observer.h
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
        )

set(DOC_FILES
        ${ROOT_DIR}/doc/references.md)