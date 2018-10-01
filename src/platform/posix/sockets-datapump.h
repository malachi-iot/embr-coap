#pragma once

#include <embr/datapump.h>
#include "exp/retry.h"
#include <netinet/in.h>
#include "../generic/malloc_netbuf.h"

namespace moducom { namespace coap {

typedef embr::TransportDescriptor<moducom::coap::NetBufDynamicExperimental, sockaddr_in> transport_descriptor_t;
typedef embr::DataPump<transport_descriptor_t> sockets_datapump_t;
typedef moducom::coap::experimental::Retry<moducom::coap::NetBufDynamicExperimental, sockaddr_in> sockets_retry_t;

extern sockets_datapump_t sockets_datapump;

}}


namespace embr { namespace experimental {

template <>
struct address_traits<sockaddr_in>
{
    static bool equals_fromto(const sockaddr_in& from, const sockaddr_in& to)
    {
        return from.sin_addr.s_addr == to.sin_addr.s_addr;
    }
};



}}

int nonblocking_datapump_setup();
void nonblocking_datapump_loop(int h, moducom::coap::sockets_datapump_t& datapump = moducom::coap::sockets_datapump);
int nonblocking_datapump_shutdown(int);

namespace moducom { namespace coap {

// combining socket handle with queue allocation
// not 100% sure we always want to do that, but seems good so far
class SocketsDatapumpHelper
{
    const int sockfd;

#ifdef FEATURE_MCCOAP_RELIABLE
    sockets_retry_t retry;
#endif

public:
    typedef sockets_datapump_t::addr_t addr_t;
    typedef sockets_datapump_t::netbuf_t netbuf_t;
    typedef sockets_datapump_t datapump_t;
    //typedef sockets_datapump_t::datapump_observer_t datapump_observer_t;

    SocketsDatapumpHelper() :
        sockfd(nonblocking_datapump_setup())
    {
    }

    ~SocketsDatapumpHelper()
    {
        nonblocking_datapump_shutdown(sockfd);
    }

    void loop(sockets_datapump_t& datapump = sockets_datapump)
    {
        nonblocking_datapump_loop(sockfd, datapump);
#ifdef FEATURE_MCCOAP_RELIABLE
        // FIX: Problematic because it leaves a lingering question, does this auto-pop
        // the ACK message *or* shall we leave it for the user to evaluate?  Elegant
        // solution is with MessageObserver pattern, but there should be an elegant
        // approach with just raw processing style also.  At present it leaves the ACK
        // in the queue.
        retry.service_ack(datapump);
        retry.service_retry(sockets_retry_t::time_traits::now(), datapump);
#endif
    }

    // dequeue input that was previously queued up from transport
    // returns NULL if no item queued
    netbuf_t* front(addr_t* addr_in, sockets_datapump_t& datapump = sockets_datapump)
    {
        return datapump.dequeue_in(addr_in);
    }

    // remove netbuf from transport in queue
    void pop(sockets_datapump_t& datapump = sockets_datapump)
    {
        datapump.dequeue_pop();
    }

    // indicates whether any netbufs have queued up from transport in
    bool empty(sockets_datapump_t& datapump = sockets_datapump)
    {
        return datapump.dequeue_empty();
    }

// queue up to send out over transport
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
    void enqueue(
            netbuf_t&& netbuf,
            const addr_t& addr_out, sockets_datapump_t& datapump = sockets_datapump)
    {
#ifdef FEATURE_MCCOAP_RELIABLE
        bool is_con = retry.is_con(netbuf);
        //datapump_observer_t* observer = &retry.enqueue(netbuf, addr_out);
#else
        //datapump_observer_t* observer = NULLPTR;
#endif

        datapump.enqueue_out(std::forward<netbuf_t>(netbuf), addr_out, observer);
    }
#else
    void enqueue(
            netbuf_t& netbuf,
            const addr_t& addr_out, sockets_datapump_t& datapump = sockets_datapump)
    {
#ifdef FEATURE_MCCOAP_RELIABLE
        bool is_con = retry.is_con(netbuf);
        //datapump_observer_t* observer = &retry.enqueue(netbuf, addr_out);
#else
        //datapump_observer_t* observer = NULLPTR;
#endif

        datapump.enqueue_out(netbuf, addr_out);
    }
#endif
};

}}
