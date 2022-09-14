#include "unit-test.h"

#include <estd/chrono.h>

#if __cplusplus >= 201103L
#ifdef ESP_IDF_TESTING
#include <embr/platform/lwip/endpoint.h>
#include <embr/platform/lwip/pbuf.h>
#include <embr/platform/lwip/istream.h>
#include <embr/platform/lwip/ostream.h>

#include <coap/platform/ip.h>


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

static void test_retry_1()
{
    embr::lwip::udp::Pcb pcb;
    embr::lwip::Pbuf buffer(128);

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

    manager.send(endpoint, time_point(estd::chrono::seconds(5)), buffer, scheduler);
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