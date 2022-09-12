#pragma once

// all timeouts/delays/etc for these COAP_ constants are in seconds
// https://tools.ietf.org/html/rfc7252#section-4.8
#define COAP_ACK_TIMEOUT        2
#define COAP_DEFAULT_LEUISURE   5

#define COAP_ACK_RANDOM_FACTOR  1.5
#define COAP_MAX_RETRANSMIT     4
#define COAP_NSTART             1
#define COAP_PROBING_RATE       1   // in bytes per second
