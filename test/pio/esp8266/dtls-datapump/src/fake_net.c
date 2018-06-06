#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/net.h"

#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
//#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>

#include <time.h>

#include <stdint.h>

// copy-paste of ESP8266 net_accept so I can troubleshoot things

#include "lwip/opt.h"

#include "lwip/api.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/igmp.h"
#include "lwip/inet.h"
#include "lwip/tcp.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "lwip/pbuf.h"
#if LWIP_CHECKSUM_ON_COPY
#include "lwip/inet_chksum.h"
#endif


/*
 * Accept a connection from a remote client
 */
int dbg_mbedtls_net_accept( mbedtls_net_context *bind_ctx,
                        mbedtls_net_context *client_ctx,
                        void *client_ip, size_t buf_size, size_t *ip_len )
{
    int ret;
    int type;

    struct sockaddr_in client_addr;

    socklen_t n = (socklen_t) sizeof( client_addr );
    socklen_t type_len = (socklen_t) sizeof( type );

    /* Is this a TCP or UDP socket? */
    if( getsockopt( bind_ctx->fd, SOL_SOCKET, SO_TYPE,
                    (void *) &type, (socklen_t *) &type_len ) != 0 ||
        ( type != SOCK_STREAM && type != SOCK_DGRAM ) )
    {
        return( MBEDTLS_ERR_NET_ACCEPT_FAILED );
    }

    if( type == SOCK_STREAM )
    {
        /* TCP: actual accept() */
        ret = client_ctx->fd = (int) accept( bind_ctx->fd,
                                             (struct sockaddr *) &client_addr, &n );
    }
    else
    {
        /* UDP: wait for a message, but keep it in the queue */
        char buf[1] = { 0 };

        ret = recvfrom( bind_ctx->fd, buf, sizeof( buf ), MSG_PEEK,
                        (struct sockaddr *) &client_addr, &n );

#if defined(_WIN32)
        if( ret == SOCKET_ERROR &&
            WSAGetLastError() == WSAEMSGSIZE )
        {
            /* We know buf is too small, thanks, just peeking here */
            ret = 0;
        }
#endif
    }

    if( ret < 0 )
    {
        printf("Undefined: net_would_block\n");
        /*
        if( net_would_block( bind_ctx ) != 0 )
            return( MBEDTLS_ERR_SSL_WANT_READ ); */

        return( MBEDTLS_ERR_NET_ACCEPT_FAILED );
    }

    /* UDP: hijack the listening socket to communicate with the client,
     * then bind a new socket to accept new connections */
    if( type != SOCK_STREAM )
    {
        struct sockaddr_in local_addr;
        int one = 1;

        if( connect( bind_ctx->fd, (struct sockaddr *) &client_addr, n ) != 0 )
            return( MBEDTLS_ERR_NET_ACCEPT_FAILED );

        client_ctx->fd = bind_ctx->fd;
        bind_ctx->fd   = -1; /* In case we exit early */

        n = sizeof( struct sockaddr_in );
        
        int ret;

        ret = getsockname( client_ctx->fd,
                         (struct sockaddr *) &local_addr, &n );
                         // != 0

        if(ret != 0)
        {
            printf("Err %d in getsockname\n", ret);
            return( MBEDTLS_ERR_NET_SOCKET_FAILED );
        }

        ret = bind_ctx->fd = (int) socket( AF_INET,
                                           SOCK_DGRAM, IPPROTO_UDP ); // < 0

        if(ret < 0)
        {
            printf("Err %d in socket()\n", ret);
            return( MBEDTLS_ERR_NET_SOCKET_FAILED );
        }

        ret = setsockopt( bind_ctx->fd, SOL_SOCKET, SO_REUSEADDR,
                        (const char *) &one, sizeof( one ) );

        if(ret  != 0)
        {
            printf("Err %d in setsockopt() - socket=%d\n",
                ret, bind_ctx->fd);
            //printf("Sock conn=%d\n", bind_ctx->fd->conn);

            return( MBEDTLS_ERR_NET_SOCKET_FAILED );
        }

        if( bind( bind_ctx->fd, (struct sockaddr *) &local_addr, n ) != 0 )
        {
            return( MBEDTLS_ERR_NET_BIND_FAILED );
        }
    }

    if( client_ip != NULL )
    {
            struct sockaddr_in *addr4 = (struct sockaddr_in *) &client_addr;
            *ip_len = sizeof( addr4->sin_addr.s_addr );

            if( buf_size < *ip_len )
                return( MBEDTLS_ERR_NET_BUFFER_TOO_SMALL );

            memcpy( client_ip, &addr4->sin_addr.s_addr, *ip_len );
    }

    return( 0 );
}