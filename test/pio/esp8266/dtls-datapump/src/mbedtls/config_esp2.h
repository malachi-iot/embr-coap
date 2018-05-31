#include_next<mbedtls/config_esp.h>

#define MBEDTLS_SSL_COOKIE_C
#define MBEDTLS_SSL_CACHE_C

#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_PROTO_DTLS

// This combined with SSL_SRV_C enables cookies
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY
