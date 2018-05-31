// pulled in from PGESP-4 as config.h
// esp-idf (or maybe PIO?) now seems to prefer 'esp_config.h' over 'config.h'

#define MBEDTLS_DEBUG_C

#include_next<mbedtls/esp_config.h>

// Pretty sure we don't need these
//#undef MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
#undef MBEDTLS_SSL_PROTO_SSL3
//#undef MBEDTLS_SSL_PROTO_TLS1

#undef MBEDTLS_DES_C
#undef MBEDTLS_BLOWFISH_C

// Might need these
//#undef MBEDTLS_ASN1_PARSE_C
//#undef MBEDTLS_ASN1_WRITE_C

// Fully experimental
#undef MBEDTLS_SSL_MAX_CONTENT_LEN
#define MBEDTLS_SSL_MAX_CONTENT_LEN             3000

// FIX: Odd that we need these here but didn't in the other PGFR-9 project

#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_PROTO_DTLS
#define MBEDTLS_CERTS_C
#define MBEDTLS_SSL_COOKIE_C
#define MBEDTLS_CTR_DRBG_C
// I don't understand - ESP32 *disables* mbedtls_net_* stuff when
// this is #defined, but on other platforms it seems to *enable* it
//#define MBEDTLS_NET_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_RSA_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_ENTROPY_C

//#undef MBEDTLS_ECDSA_C
//#undef MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED
//#undef MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED

//#define MBEDTLS_TIMING_C

// Can't go to level 3, something is wrong with SSLContext
#define MBEDTLS_CPP     2

// This combined with SSL_SRV_C enables cookies
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY

