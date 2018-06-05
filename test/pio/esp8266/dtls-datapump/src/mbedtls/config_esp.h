// Not getting here, damn
#error GOT HERE

#include_next<mbedtls/config_esp.h>

#define MBEDTLS_NET_C

#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_CERTS_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_RSA_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_ENTROPY_C

#define MBEDTLS_SSL_COOKIE_C
#define MBEDTLS_SSL_CACHE_C

#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_PROTO_DTLS

// This combined with SSL_SRV_C enables cookies
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY
