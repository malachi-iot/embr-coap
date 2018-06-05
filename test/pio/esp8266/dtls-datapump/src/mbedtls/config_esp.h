#include_next<mbedtls/config_esp.h>

//#define MBEDTLS_NET_C // Pretty sure this is for POSIX compliant scenarios only
#define MBEDTLS_DEBUG_C             // Req# 6 (probably)

//#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_CTR_DRBG_C          // Req# 6

// these two are both needed to enable test certificates
#define MBEDTLS_SELF_TEST           // Needed for Req# 9 certs
#define MBEDTLS_CERTS_C             // Req# 9

#define MBEDTLS_X509_CRT_PARSE_C    // Req# 7
#define MBEDTLS_RSA_C               // Req# 8
#define MBEDTLS_PEM_PARSE_C         // Req# 10
#define MBEDTLS_ENTROPY_C           // Req# 5

#define MBEDTLS_SSL_COOKIE_C        // Req# 3
//#define MBEDTLS_SSL_CACHE_C

#define MBEDTLS_SSL_SRV_C           // Req# 1
#define MBEDTLS_SSL_PROTO_DTLS      // Req# 2

// This combined with SSL_SRV_C enables cookies
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY
