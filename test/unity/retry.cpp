/**
 * 
 * References:
 * 
 * 1. lwip.fandom.com/wiki/Raw/UDP
 */

#include "unit-test.h"

#include <estd/chrono.h>

#if __cplusplus >= 201103L
#ifdef ESP_IDF_TESTING
#include "esp_log.h"

#include <embr/platform/lwip/endpoint.h>
#include <embr/platform/lwip/pbuf.h>
#include <embr/platform/lwip/istream.h>
#include <embr/platform/lwip/ostream.h>

#include <coap/platform/ip.h>

// If LwIP loopback capability is present, then consider enabling our loopback tests
#if LWIP_HAVE_LOOPIF && LWIP_LOOPBACK_MAX_PBUFS
#ifndef FEATURE_COAP_LWIP_LOOPBACK_TESTS
#define FEATURE_COAP_LWIP_LOOPBACK_TESTS 1
#endif
#endif


// DEBT: Put this into embr proper
namespace embr::lwip::experimental {

// NOTE: Evidently we need this operator to reside in the namespace matching
// its class
template <bool use_pointer>
bool operator ==(const Endpoint<use_pointer>& lhs, const Endpoint<use_pointer>& rhs)
{
    // *lhs.address() == *rhs.address();
    // We need an ipv4 vs ipv6 version to properly compare addresses
    bool address_match = ip_addr_cmp(lhs.address(), rhs.address());

    return lhs.port() == rhs.port() && address_match;
}

}

#include <exp/retry/factory.h>

namespace embr { namespace coap { namespace experimental {

template <>
struct StreambufProvider<embr::lwip::Pbuf>
{
    typedef embr::lwip::upgrading::basic_opbuf_streambuf<uint8_t> ostreambuf_type;
    typedef embr::lwip::upgrading::basic_ipbuf_streambuf<uint8_t> istreambuf_type;
};

}}}

#endif

#include <exp/retry.h>

using namespace embr;

// DEBT: Really we want to test against any LwIP capable target
#ifdef ESP_IDF_TESTING
#include <embr/platform/lwip/transport.hpp>

//typedef lwip::experimental::TransportUdp<> transport_type;
// DEBT: const_buffer_type still in flux
struct transport_type : lwip::experimental::TransportUdp<>
{
    typedef lwip::experimental::TransportUdp<> base_type;
    typedef typename base_type::buffer_type const_buffer_type;

    ESTD_CPP_FORWARDING_CTOR(transport_type);
};

typedef typename transport_type::endpoint_type endpoint_type;

typedef estd::chrono::freertos_clock clock_type;
typedef typename clock_type::time_point time_point;
typedef embr::internal::layer1::Scheduler<8, embr::internal::scheduler::impl::Function<time_point> > scheduler_type;

static void udp_receive(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
    puts("Got here");
}

static void test_retry_1()
{
    static const char* TAG = "test_retry_1";

    embr::lwip::udp::Pcb pcb;
    embr::lwip::Pbuf buffer(128);

    pcb.alloc();

    ip_addr_t addr;

    //ip4_addr_set_u32(&addr, IP_LOOPBACKNET);

    // Have to do a little dance because ipv6 might be present also,
    // so ip_2_ip4 macro is needed to navigate that
    ip4_addr_set_loopback(ip_2_ip4(&addr));

    //addr.u_addr.ip4 = PP_HTONL(IPADDR_LOOPBACK);
    

    scheduler_type scheduler;
    endpoint_type endpoint(&addr, embr::coap::IP_PORT);

    // DEBT: Add copy/move constructor to TransportUdp
    //transport_type transport(pcb);

    coap::experimental::retry::Manager<clock_type, transport_type> manager(pcb);

    //pbuf_ref(buffer);

#if FEATURE_COAP_LWIP_LOOPBACK_TESTS
    // Tried doing a separate pcb_recv to avoid ref == 1 errors, but no
    // dice.  It's more tied to tracker behavior
    embr::lwip::udp::Pcb pcb_recv;

    pcb_recv.alloc();
    pcb_recv.bind(endpoint.address(), endpoint.port());
    pcb_recv.recv(udp_receive);
    pcb_recv.free();
#endif

    ESP_LOGI(TAG, "ref=%d", buffer.pbuf()->ref);

    // Turns out you can only have pbuf ref == 1 when doing a send, which is problematic
    // to our retry model which expects to hold on to the pbuf to send it again later.
    // 'tracker' naturally bumps this ref up, and so we have a problem
    // [1] indicates udp_send doesn't free the thing.  Also, udp.c source code
    // indicates "p is still referenced by the caller, and will live on"
    manager.send(endpoint, time_point(estd::chrono::seconds(5)),
        std::move(buffer), scheduler);

    pcb.free();
}

#endif

#endif

#ifdef ESP_IDF_TESTING
TEST_CASE("retry tests", "[retry]")
#else
void test_retry()
#endif
{
#if __cplusplus >= 201103L
    RUN_TEST(test_retry_1);
#endif
}