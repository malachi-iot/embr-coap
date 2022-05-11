//
// Created by malachi on 10/20/17.
//

// Enhanced std::clog diagnostics
//#define DEBUG2


#ifndef SRC_COAP_H
#define SRC_COAP_H

#include <stdint.h>
#include <stdlib.h>

#include <estd/internal/platform.h> // DEBT: Need to do this because we forgot to do it inside new.h
#include <estd/new.h>
#include <estd/type_traits.h>

#include "coap/platform.h"
#include "coap/option.h"
#include "coap/header.h"

namespace embr { namespace coap {

// all timeouts/delays/etc for these COAP_ constants are in seconds
// https://tools.ietf.org/html/rfc7252#section-4.8
#define COAP_ACK_TIMEOUT        2
#define COAP_DEFAULT_LEUISURE   5

#define COAP_ACK_RANDOM_FACTOR  1.5
#define COAP_MAX_RETRANSMIT     4
#define COAP_NSTART             1
#define COAP_PROBING_RATE       1   // in bytes per second

// https://tools.ietf.org/html/rfc7252#section-4.8.2
#define COAP_MAX_TRANSMIT_SPAN  COAP_ACK_TIMEOUT * ((2 ** COAP_MAX_RETRANSMIT) - 1) * COAP_ACK_RANDOM_FACTOR
#define COAP_MAX_TRANSMIT_WAIT  COAP_ACK_TIMEOUT * ((2 ** (COAP_MAX_RETRANSMIT + 1)) - 1) * COAP_ACK_RANDOM_FACTOR
#define COAP_MAX_LATENCY        100
#define COAP_PROCESSING_DELAY   COAP_ACK_TIMEOUT
#define COAP_MAX_RTT            (2 * COAP_MAX_LATENCY) + COAP_PROCESSING_DELAY
#define COAP_EXCHANGE_LIFETIME  COAP_MAX_TRANSMIT_SPAN + (2 * COAP_MAX_LATENCY) + COAP_PROCESSING_DELAY
#define COAP_NON_LIFETIME       COAP_MAX_TRANSMIT_SPAN + COAP_MAX_LATENCY



#define COAP_OPTION_DELTA_POS   4
#define COAP_OPTION_DELTA_MASK  15
#define COAP_OPTION_LENGTH_POS  0
#define COAP_OPTION_LENGTH_MASK 15

#define COAP_EXTENDED8_BIT_MAX  (255 - 13)
#define COAP_EXTENDED16_BIT_MAX (0xFFFF - 269)

#define COAP_PAYLOAD_MARKER     0xFF

#define COAP_FEATURE_SUBSCRIPTIONS


namespace internal {

// FIX: Need a better name.  Root class of helpers for message-level
// Encoder/Decoder operations
class Root
{
public:
    // for IPipelineWriter, mainly
    typedef uint8_t boundary_t;

    // The end of header, token, options are marked with segment
    // NOTE: Not fully functional yet
    static CONSTEXPR boundary_t boundary_segment = 3;
    // the end of the entire message marked with this one
    static CONSTEXPR boundary_t boundary_message = 4;

    enum State
    {
        Uninitialized,
        Header,
        HeaderDone,
        TokenStart, // currently unused
        Token,
        /// Emit always, even if tkl = 0
        TokenDone,
        OptionsStart,
        Options,
        OptionsDone, // all options are done being processed
        Payload,
        PayloadDone,
        // Denotes completion of entire CoAP message
        Done,
    };
};

typedef Root::State root_state_t;
typedef Root _root_state_t;

}

const char* get_description(Option::Numbers number);
const char* get_description(internal::root_state_t state);

namespace experimental {


//typedef CoAP::OptionExperimentalDeprecated::ValueFormats option_value_format_t;
typedef Option::ContentFormats option_content_format_t;

typedef Header::TypeEnum header_type_t;
typedef Header::Code::Codes header_response_code_t;
typedef Header::RequestMethodEnum header_request_code_t;

typedef Option::ExtendedMode extended_mode_t;
typedef Option _extended_mode_t;

const char* get_description(Option::State state);

}

}}


#endif //SRC_COAP_H
