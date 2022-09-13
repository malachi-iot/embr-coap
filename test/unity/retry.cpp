#include "unit-test.h"

#include <estd/chrono.h>

#if __cplusplus >= 201103L
#ifdef ESP_IDF_TESTING
#include <embr/platform/lwip/endpoint.h>
#include <embr/platform/lwip/pbuf.h>
#include <embr/platform/lwip/istream.h>
#include <embr/platform/lwip/ostream.h>

using Endpoint = embr::lwip::experimental::Endpoint<true>;

bool operator ==(const Endpoint& lhs, const Endpoint& rhs)
{
    return false;
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

typedef estd::chrono::freertos_clock clock_type;

static void test_retry_1()
{
    coap::experimental::retry::Manager<clock_type, transport_type> manager;
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