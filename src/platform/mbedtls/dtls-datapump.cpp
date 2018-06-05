//
// Created by malachi on 5/30/18.
//

// Adapting from https://github.com/ARMmbed/mbedtls/blob/development/programs/ssl/dtls_server.c

// MBEDTLS_CONFIG_FILE specifies this, but experimenting we need to help it along
// for PIO.  Probably chain+ ldf will resolve without this help, but for now
// let's not take any chances
//#include "mbedtls/config_esp.h"

#include <mc/opts-internal.h>

#ifdef FEATURE_MCCOAP_MBEDTLS

#include "dtls-datapump.h"

extern "C" {

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#ifdef ESP8266
#include "esp_common.h" // needed to resolve os_printf as referenced from mbedtls/platform.h for this platform
#endif

#else
#include <stdio.h>
#define mbedtls_printf     printf
#define mbedtls_fprintf    fprintf
#define mbedtls_time_t     time_t
#endif

// MB: Hack this in since we provide our own timing
#define MBEDTLS_TIMING_C

// As per https://tls.mbed.org/api/config_8h.html#ae73dbeb395510070a756ed6689459b30
// this is only available on POSIX type systems.  ESP32 appears to *only* provide
// drop-in compatility for MBEDTLS_NET_C by *disabling* MBEDTLS_NET_C flag for
// system compilation.
// 
// Confusing, but there it is.  Re-enable it here since we *do* use those
// APIs - mainly to pass the following big-define-test
#if defined(ESP32) || defined(ESP8266)
#define MBEDTLS_NET_C
#endif

#if !defined(MBEDTLS_SSL_SRV_C) || !defined(MBEDTLS_SSL_PROTO_DTLS) ||    \
    !defined(MBEDTLS_SSL_COOKIE_C) || !defined(MBEDTLS_NET_C) ||          \
    !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_CTR_DRBG_C)

#error "Missing requisite flag - see dtls-datapump.cpp (batch 1)"

#endif

#if !defined(MBEDTLS_X509_CRT_PARSE_C) || !defined(MBEDTLS_RSA_C) ||      \
    !defined(MBEDTLS_CERTS_C) || !defined(MBEDTLS_PEM_PARSE_C) ||         \
    !defined(MBEDTLS_TIMING_C)

#error "Missing requisite flag - see dtls-datapump.cpp (batch 2)"

#endif

#if defined(_WIN32)
#include <windows.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cookie.h"
#if MBEDTLS_VERSION_NUMBER > 0x02010000
#include "mbedtls/net_sockets.h"
#else
#include "mbedtls/net.h"
#endif
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"

#if defined(MBEDTLS_SSL_CACHE_C)
#include "mbedtls/ssl_cache.h"
#endif

#ifdef ESP8266
// as per https://github.com/espressif/ESP8266_RTOS_SDK/issues/77
// Not 100% sure we need it for DTLS, but definitely for TLS
unsigned int max_content_len = 3072;
#endif

} // extern "C"

#define READ_TIMEOUT_MS 10000   /* 5 seconds */
#define DEBUG_LEVEL 0

namespace moducom { namespace coap {

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    ((void) level);

    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: %s", file, line, str );
    fflush(  (FILE *) ctx  );
}

// FIX: TEMPORARILY making these static globals while I figure out a strategy to avoid making
// all consumers of dtls-datapump.h include all the darned mbedtls files too
    mbedtls_net_context listen_fd, client_fd;
    mbedtls_ssl_cookie_ctx cookie_ctx;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;
    mbedtls_timing_delay_context timer;
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_context cache;
#endif
    uint8_t buf[1024];

int Dtls::setup()
{
    int ret;
    const char *pers = "dtls_server";


    mbedtls_net_init( &listen_fd );
    mbedtls_net_init( &client_fd );
    mbedtls_ssl_init( &ssl );
    mbedtls_ssl_config_init( &conf );
    mbedtls_ssl_cookie_init( &cookie_ctx );
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_init( &cache );
#endif
    mbedtls_x509_crt_init( &srvcert );
    mbedtls_pk_init( &pkey );
    mbedtls_entropy_init( &entropy );
    mbedtls_ctr_drbg_init( &ctr_drbg );

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold( DEBUG_LEVEL );
#endif

    /*
     * 1. Load the certificates and private RSA key
     */
    printf( "\n  . Loading the server cert. and key..." );
    fflush( stdout );

    /*
     * This demonstration program uses embedded test certificates.
     * Instead, you may want to use mbedtls_x509_crt_parse_file() to read the
     * server and CA certificates, as well as mbedtls_pk_parse_keyfile().
     */
    ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) mbedtls_test_srv_crt,
                          mbedtls_test_srv_crt_len );
    if( ret != 0 )
    {
        printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
        return ret;
    }

    ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) mbedtls_test_cas_pem,
                          mbedtls_test_cas_pem_len );
    if( ret != 0 )
    {
        printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
        return ret;
    }

    ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) mbedtls_test_cas_pem,
                          mbedtls_test_cas_pem_len );
    if( ret != 0 )
    {
        printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
        return ret;
    }

    ret =  mbedtls_pk_parse_key( &pkey, (const unsigned char *) mbedtls_test_srv_key,
                         mbedtls_test_srv_key_len, NULL, 0 );
    if( ret != 0 )
    {
        printf( " failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret );
        return ret;
    }

    printf( " ok\n" );

    /*
     * 2. Setup the "listening" UDP socket
     */
    printf( "  . Bind on udp/*/4433 ..." );
    fflush( stdout );

    if( ( ret = mbedtls_net_bind( &listen_fd, NULL, "4433", MBEDTLS_NET_PROTO_UDP ) ) != 0 )
    {
        printf( " failed\n  ! mbedtls_net_bind returned %d\n\n", ret );
        return ret;
    }

    printf( " ok\n" );

    /*
     * 3. Seed the RNG
     */
    printf( "  . Seeding the random number generator..." );
    fflush( stdout );

    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        return ret;
    }

    printf( " ok\n" );

    /*
     * 4. Setup stuff
     */
    printf( "  . Setting up the DTLS data..." );
    fflush( stdout );

    if( ( ret = mbedtls_ssl_config_defaults( &conf,
                    MBEDTLS_SSL_IS_SERVER,
                    MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret );
        return ret;
    }

    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    mbedtls_ssl_conf_dbg( &conf, my_debug, stdout );

#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_conf_session_cache( &conf, &cache,
                                   mbedtls_ssl_cache_get,
                                   mbedtls_ssl_cache_set );
#endif

    mbedtls_ssl_conf_ca_chain( &conf, srvcert.next, NULL );
   if( ( ret = mbedtls_ssl_conf_own_cert( &conf, &srvcert, &pkey ) ) != 0 )
    {
        printf( " failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret );
        return ret;
    }

    if( ( ret = mbedtls_ssl_cookie_setup( &cookie_ctx,
                                  mbedtls_ctr_drbg_random, &ctr_drbg ) ) != 0 )
    {
        printf( " failed\n  ! mbedtls_ssl_cookie_setup returned %d\n\n", ret );
        return ret;
    }

    // As per https://tls.mbed.org/kb/how-to/dtls-tutorial, this should happen
    // on the 'main thread' so beware
    mbedtls_ssl_conf_dtls_cookies( &conf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check,
                               &cookie_ctx );

    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
    {
        printf( " failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret );
        return ret;
    }

    mbedtls_ssl_set_timer_cb( &ssl, &timer, mbedtls_timing_set_delay,
                                            mbedtls_timing_get_delay );

    printf( " ok\n" );

    return ret;
}


// TODO: will take datapump either as an input or more of an instance variable
bool Dtls::loop(int* _ret)
{
    // FIX: Since we converted this from a suite of goto statements, keep these as static
    // until we truly scope it out and switch to DataPump
    //static unsigned char client_ip[16] = { 0 };
    addr_t client_ip = { 0 };
    static size_t cliip_len;
    static int len;

    int& ret = *_ret;

    error(ret);

    // QUESTION: Can we use mbedtls_net_bind for new outgoing UDP traffic, or must we
    // use mbedtls_net_connect?  Furthermore, I assume mbedtls_net_connect doesn't
    // *actually* create a connection with UDP and therefore is nonblocking, but that
    // is an assumption - an assumption implied by dtls_client.c
    // https://github.com/ARMmbed/mbedtls/blob/development/programs/ssl/dtls_client.c
    // TODO: Perform a datapump.dequeue_empty()

    mbedtls_net_free( &client_fd );

    mbedtls_ssl_session_reset( &ssl );

    /*
     * 3. Wait until a client connects
     */
    printf( "  . Waiting for a remote connection ..." );
    fflush( stdout );

    if( ( ret = mbedtls_net_accept( &listen_fd, &client_fd,
                    client_ip, sizeof( client_ip ), &cliip_len ) ) != 0 )
    {
        printf( " failed\n  ! mbedtls_net_accept returned %d\n\n", ret );
        return false;
    }

    /* For HelloVerifyRequest cookies */
    if( ( ret = mbedtls_ssl_set_client_transport_id( &ssl,
                    client_ip, cliip_len ) ) != 0 )
    {
        printf( " failed\n  ! "
                "mbedtls_ssl_set_client_transport_id() returned -0x%x\n\n", -ret );
        return false;
    }

    mbedtls_ssl_set_bio( &ssl, &client_fd,
                         mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout );

    printf( " ok\n" );

    /*
     * 5. Handshake
     */
    printf( "  . Performing the DTLS handshake..." );
    fflush( stdout );

    do ret = mbedtls_ssl_handshake( &ssl );
    while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE );

    if( ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED )
    {
        printf( " hello verification requested\n" );
        ret = 0;
        return true;
    }
    else if( ret != 0 )
    {
        printf( " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret );
        return true;
    }

    printf( " ok\n" );

    /*
     * 6. Read the echo Request
     */
    printf( "  < Read from client:" );
    fflush( stdout );

    len = sizeof( buf ) - 1;
    memset( buf, 0, sizeof( buf ) );

    // FIX: Seems that MBEDTLS_ERR_SSL_WANT_READ wants more buffer space
    // and this code doesn't accomodate that
    do ret = mbedtls_ssl_read( &ssl, buf, len );
    while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE );

    if( ret <= 0 )
    {
        switch( ret )
        {
            case MBEDTLS_ERR_SSL_TIMEOUT:
                printf( " timeout\n\n" );
                return true;

            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                printf( " connection was closed gracefully\n" );
                ret = 0;
                goto close_notify;

            default:
                printf( " mbedtls_ssl_read returned -0x%x\n\n", -ret );
                return true;
        }
    }

    len = ret;

    //if(dtls_server_request_handler != NULL)
      //  dtls_server_request_handler(buf, len, NULL);

    // TODO: Turn these printfs into debug-only
    printf( " %d bytes read\n\n%s\n\n", len, buf );

    /*
     * 7. Write the 200 Response
     */
    printf( "  > Write to client:" );
    fflush( stdout );

    //if(dtls_server_response_handler != NULL)
      //  len = dtls_server_response_handler(buf, sizeof(buf), NULL);

    // FIX: according to https://tls.mbed.org/api/ssl_8h.html,
    // buf and len need to be updated with ret when partial writes happen
    do ret = mbedtls_ssl_write( &ssl, buf, len );
    while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE );

    if( ret < 0 )
    {
        printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
        return false;
    }

    len = ret;
    printf( " %d bytes written\n\n%s\n\n", len, buf );

    /*
     * 8. Done, cleanly close the connection
     */
close_notify:
    printf( "  . Closing the connection..." );

    /* No error checking, the connection might be closed already */
    do ret = mbedtls_ssl_close_notify( &ssl );
    while( ret == MBEDTLS_ERR_SSL_WANT_WRITE );
    ret = 0;

    printf( " done\n" );

    return true;
}

void Dtls::error(int ret)
{
#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        printf( "Last error was: %d - %s\n\n", ret, error_buf );
    }
#endif
}


void Dtls::shutdown()
{
mbedtls_net_free( &client_fd );
    mbedtls_net_free( &listen_fd );

    mbedtls_x509_crt_free( &srvcert );
    mbedtls_pk_free( &pkey );
    mbedtls_ssl_free( &ssl );
    mbedtls_ssl_config_free( &conf );
    mbedtls_ssl_cookie_free( &cookie_ctx );
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_free( &cache );
#endif
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );
}


}}

#endif
