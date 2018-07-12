#pragma once

// non-coap-specific DTLS helper class
// stuffing into mbedtls namespace but in theory could be a gneeric helper
// (abstracting across crypto libs).  But that's a pretty big bite to chew,
// so start with just this

#include <mc/opts-internal.h>


extern "C" {
#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif


#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl_cookie.h"
#include "mbedtls/timing.h"
#if defined(MBEDTLS_SSL_CACHE_C)
#include "mbedtls/ssl_cache.h"
#endif

}

namespace mbedtls {

// TODO: These calls all default to blocking, 
// but we definitely want a non blocking variety
// a slight complexity (only slight) in that the 
// transport itself seems to determine whether
// non-blocking modes are available
class DtlsServer
{
    mbedtls_ssl_context ssl;

    // NOTE: Pretty sure we can dump conf after initial setup, but keeping for now
    mbedtls_ssl_config conf; 

    // perhaps some of these we can drop after handshake also (maybe we need a
    // handshake context)
    mbedtls_ssl_cookie_ctx cookie_ctx;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;
    mbedtls_timing_delay_context timer;

#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_context cache;
#endif

    // as indicated by dtls_server.c examples
    // TODO: In order for 'observe' functionality we'll need to be able to construct these
    // from scratch.  Standard request/response stuff we should be good echoing back what
    // we got on input
    typedef uint8_t addr_t[16];

public:
    DtlsServer();
    ~DtlsServer();
    
    int load_certs();

    // setup also initiates the 'bind', might wanna break taht out 
    // seperately
    int setup();

    class HandshakeContext
    {

    };

    int err();

    bool handshake(HandshakeContext context, int* _ret);
    void handshake();

    // check to see if any other datagrams are ready for us to read
    bool is_pending()
    {
        return mbedtls_ssl_check_pending(&ssl) != 0;
    }

    template <class TNetBuf>
    void write(TNetBuf& netbuf);

    // returns false only when connection was closed underneath the read
    template <class TNetBuf>
    bool read(TNetBuf& netbuf);

    void close();
};


template <class TSubject>
class DtlsServerSubject
{

};

}