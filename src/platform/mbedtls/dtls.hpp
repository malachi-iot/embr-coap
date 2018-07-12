#include "dtls.h"

//
// Created by malachi on 5/30/18.
//

// Adapting from https://github.com/ARMmbed/mbedtls/blob/development/programs/ssl/dtls_server.c

// MBEDTLS_CONFIG_FILE specifies this, but experimenting we need to help it along
// for PIO.  Probably chain+ ldf will resolve without this help, but for now
// let's not take any chances
//#include "mbedtls/config_esp.h"

extern "C" {

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
#if defined(ESP32) || defined(ESP8266) || defined(IDF_VER)
#define MBEDTLS_NET_C

// do some non-MBEDTLS_NET_C stuff here also
#define FREERTOS // TODO: Grab proper FreeRTOS flag from FreeRTOS itself, if we can
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#endif

#if !defined(MBEDTLS_SSL_SRV_C) || !defined(MBEDTLS_SSL_PROTO_DTLS)

#error "Missing requisite flag - see dtls-datapump.cpp (batch 1)"

#endif

#if !defined(MBEDTLS_SSL_COOKIE_C) || !defined(MBEDTLS_NET_C)

#error "Missing requisite flag - see dtls-datapump.cpp (batch 2)"

#endif


#if !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_CTR_DRBG_C)

#error "Missing requisite flag - see dtls-datapump.cpp (batch 3)"

#endif

#if !defined(MBEDTLS_X509_CRT_PARSE_C) || !defined(MBEDTLS_RSA_C)

#error "Missing requisite flag - see dtls-datapump.cpp (batch 4)"

#endif

#if !defined(MBEDTLS_CERTS_C) || !defined(MBEDTLS_PEM_PARSE_C)

#error "Missing requisite flag - see dtls-datapump.cpp (batch 5)"

#endif

#if !defined(MBEDTLS_TIMING_C)

#error "Missing requisite flag - see dtls-datapump.cpp (batch 6)"

#endif


#if defined(_WIN32)
#include <windows.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if MBEDTLS_VERSION_NUMBER > 0x02010000
#include "mbedtls/net_sockets.h"
#else
#include "mbedtls/net.h"
#endif
#include "mbedtls/error.h"
#include "mbedtls/debug.h"

} // extern "C"

namespace mbedtls {

// FIX: TEMPORARILY making these static globals while I figure out a strategy to avoid making
// all consumers of dtls.h include the sockets code, just cause I think it may cause
// collisions.  Been a while though, don't fully recall
static mbedtls_net_context listen_fd, client_fd;

DtlsServer::DtlsServer()
{
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

}


int DtlsServer::load_certs()
{
    int ret;

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

    return 0;
}


static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    ((void) level);

    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: %s", file, line, str );
    fflush(  (FILE *) ctx  );
}

static void error(int ret)
{
#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        printf( "Last error was: %x - %s\n\n", ret, error_buf );
    }
#endif
}



int DtlsServer::setup()
{
    int ret;
    const char *pers = "dtls_server";

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


bool DtlsServer::handshake(HandshakeContext context, int* _ret)
{
    //static unsigned char client_ip[16] = { 0 };
    addr_t client_ip = { 0 };
    size_t cliip_len;

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
     * FIX: make this nonblocking at some point
     */
    printf( "  . Waiting for a remote connection ..." );
    fflush( stdout );

    //if( ( ret = dbg_mbedtls_net_accept( &listen_fd, &client_fd,
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
        return false;
    }

    return true;
}


void DtlsServer::handshake()
{
    HandshakeContext context;
    int ret;
    bool handshake_ok;

    do
    {
        handshake_ok = handshake(context, &ret);
    }
    // while still performing handshake and we have no errors
    while(handshake_ok && ret == 0);

    // todo: return proper bool to indicate handshake success or fail
}

template <class TNetBuf>
void DtlsServer::write(TNetBuf& netbuf)
{

}

template <class TNetBuf>
bool DtlsServer::read(TNetBuf& netbuf)
{
    int ret;
    // NOTE: These are embr NetBuf's, not mc-mem NetBuf's (same concept, but
    // embr is the newer, better version)
    unsigned char* buf = netbuf.data();
    int len = netbuf.size();

    // TODO: Doesn't handle partial read of ret just yet
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
                // NOTE: this was goto close_notify before
                return false;

            default:
                printf( " mbedtls_ssl_read returned -0x%x\n\n", -ret );
                return true;
        }
    }

    return true;
}


void DtlsServer::close()
{
    int ret;

    printf( "  . Closing the connection..." );

    /* No error checking, the connection might be closed already */
    do ret = mbedtls_ssl_close_notify( &ssl );
    while( ret == MBEDTLS_ERR_SSL_WANT_WRITE );

    printf( " done\n" );
}



DtlsServer::~DtlsServer()
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

}