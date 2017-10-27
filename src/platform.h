#pragma once

#define DEBUG

// TODO: find "most" standardized version of this
#ifdef __CPP11__
#define OVERRIDE override
#define CONSTEXPR constexpr
#define NULLPTR nullptr
#else
#define OVERRIDE
#define CONSTEXPR const
#define NULLPTR NULL
#endif

#if __ADSPBLACKFIN__ || defined(_MSC_VER)
#define PACKED
#define USE_PRAGMA_PACK
#else
#define PACKED __attribute__ ((packed))
#endif


#if __ADSPBLACKFIN__ || defined(_MSC_VER)
#define COAP_HOST_LITTLE_ENDIAN
#elif defined(__GNUC__)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define COAP_HOST_BIG_ENDIAN
#else
#define COAP_HOST_LITTLE_ENDIAN
#endif

#endif

// TODO: Make this generate log warnings or something
#define ASSERT(expected, actual)

#ifdef _MSC_VER
#include <iostream>
// This will generate warnings but the program will keep going
#define ASSERT_WARN(expected, actual, message)
// This will generate a warning and issue a 'return' statement immediately
#define ASSERT_ABORT(expected, actual, message)
// This will halt the program with a message
#define ASSERT_ERROR(expected, actual, message) if((expected) != (actual)) \
{ ::std::cerr << "ERROR: (" << __func__ << ") " << message << ::std::endl; }
#else
// This will generate warnings but the program will keep going
#define ASSERT_WARN(expected, actual, message)
// This will generate a warning and issue a 'return' statement immediately
#define ASSERT_ABORT(expected, actual, message)
// This will halt the program with a message
#define ASSERT_ERROR(expected, actual, message)
#endif


// NOTE: untested old trick for byte swapping without a temp variable
#define COAP_SWAP_2_BYTES(val0, val1) { val0 ^= val1; val1 ^= val0; val0 ^= val1; }

#ifdef COAP_HOST_BIG_ENDIAN
#define COAP_HTONS(int_short) int_short
#define COAP_NTOHS(int_short) int_short
#define COAP_NTOH_2_BYTES(val0, val1)    {}
// network order bytes
// Never mind, doing bit shifting *this* way always produces the desired
// result regardless of endianness
//#define COAP_UINT16_FROM_NBYTES(val0, val1) (((uint16_t)val0) << 8 | val1)
#elif defined(COAP_HOST_LITTLE_ENDIAN)
#define COAP_HTONS(int_short) SWAP_16(int_short)
#define COAP_NTOHS(int_short) SWAP_16(int_short)
//#define COAP_UINT16_FROM_NBYTES(val0, val1) (((uint16_t)val1) << 8 | val0)
#define COAP_NTOH_2_BYTES(val0, val1)    COAP_SWAP_2_BYTES(val0, val1)
#else
#error "Unknown processor endianness"
#endif



