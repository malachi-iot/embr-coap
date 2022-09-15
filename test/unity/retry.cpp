/**
 * 
 * References:
 * 
 * 1. lwip.fandom.com/wiki/Raw/UDP
 */

#include "unit-test.h"

#include <estd/chrono.h>
#include <estd/semaphore.h>

#if __cplusplus >= 201103L
#ifdef ESP_IDF_TESTING
#include "esp_log.h"

#include "lwip/tcpip.h"

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

#if LWIP_TCPIP_CORE_LOCKING != 1
#error Need LWIP_TCPIP_CORE_LOCKING feature
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
typedef coap::experimental::retry::Manager<clock_type, transport_type> manager_type;

static ip_addr_t loopback_addr;
constexpr static int port = 10000;

static estd::freertos::counting_semaphore<1, true> signal1;


// loopback:port+1 destination
static void udp_ack_receive(void* arg, struct udp_pcb* _pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
    static const char* TAG = "udp_ack_receive";

    ESP_LOGI(TAG, "entry");

    embr::lwip::udp::Pcb pcb(_pcb);
    auto manager = (manager_type*) arg;

    signal1.release();

    ESP_LOGI(TAG, "exit");
}

// NOTE: Before consolidating into send_ack, we were getting odd behavior where the packet
// seemed to send and be looped back, but udp_ack_receive never picked it up.  Furthermore,
// that was accompanied by a ping going over loopback.  Never determined why that happened.
static void send_ack(embr::lwip::udp::Pcb& pcb, embr::coap::Header incoming_header)
{
    static const char* TAG = "send_ack";

    ESP_LOGI(TAG, "entry: header.mid=%x", incoming_header.message_id());

    // In this instance, auto-allating Pbuf is perfect
    embr::lwip::Pbuf pbuf(4);   // Header is 4 bytes
    embr::coap::Header& header = incoming_header;

    header.type(embr::coap::Header::Acknowledgement);

    // Copy header into outgoing pbuf
    // DEBT: Make a 'take' helper method for embr::lwip::udp::Pcb, and
    // consider omitting 'len' since it's expected to match buf->tot_len
    pbuf_take(pbuf, &header, 4);

    err_t result = pcb.send(pbuf, &loopback_addr, port + 1);

    ESP_LOGD(TAG, "sent: result=%d", result);
}

// loopback:port destination
static void udp_resent_receive(void* arg, struct udp_pcb* _pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
    static const char* TAG = "udp_resent_receive";

    ESP_LOGI(TAG, "entry");

    embr::lwip::udp::Pcb pcb(_pcb);
    
    // DEBT: Would vastly prefer PbufBase here and maybe in general,
    // but streams are kinda hard wired to Pbuf
    embr::lwip::Pbuf pbuf(p);
    embr::coap::Header header = embr::coap::experimental::get_header(pbuf);

    // Send ACK back to test_retry_1::pcb
    
    ESP_LOGD(TAG, "sending ACK phase 0");

    // FIX: 
    // -- With LWIP_TCPIP_CORE_LOCKING == 0:
    // LwIP logs indicate this is sent and received again on loopback,
    // yet udp_ack_receive doesn't activate
    // -- With LWIP_TCPIP_CORE_LOCKING == 1:
    // We never get past LOCK_TCPIP_CORE.  So, presumably we can't run that
    // in recv function - maybe because we're already in TCPIP core scope?

    //LOCK_TCPIP_CORE();
    send_ack(pcb, header);
    //UNLOCK_TCPIP_CORE();
    
    ESP_LOGI(TAG, "exit");
}


static void setup_outgoing_packet(embr::lwip::Pbuf& buffer)
{
    
}


static void test_retry_1_worker(void* parameter)
{
    static const char* TAG = "test_retry_1_worker";

    ESP_LOGI(TAG, "entry");

    embr::lwip::udp::Pcb pcb;
    embr::lwip::Pbuf buffer(128);

    pcb.alloc();

    ESP_LOGD(TAG, "pcb.has_pcb()=%d", pcb.has_pcb());

    scheduler_type scheduler;
    endpoint_type endpoint(&loopback_addr, port);

    // DEBT: Add copy/move constructor to TransportUdp
    //transport_type transport(pcb);

    manager_type manager(pcb);

    LOCK_TCPIP_CORE();  // Just experimenting, this lock/unlock doesn't seem to help
    pcb.bind(endpoint.address(), port + 1);
    //pcb.bind(IP_ADDR_ANY, port + 1);  // Doesn't make a difference
    pcb.recv(udp_ack_receive, &manager);
    UNLOCK_TCPIP_CORE();

    //pbuf_ref(buffer);

    ESP_LOGV(TAG, "ref=%d", buffer.pbuf()->ref);

    // [1] indicates udp_send doesn't free the thing.  Also, udp.c source code
    // indicates "p is still referenced by the caller, and will live on"

    // DEBT: This really needs to be put into TransportUdp
    LOCK_TCPIP_CORE();
    
    manager.send(endpoint, time_point(estd::chrono::seconds(5)),
        std::move(buffer), scheduler);

    UNLOCK_TCPIP_CORE();

    ESP_LOGD(TAG, "data packet sent, now waiting");

    signal1.try_acquire_for(estd::chrono::milliseconds(1000));

    pcb.free();

    ESP_LOGI(TAG, "exit");

    vTaskDelete(NULL);
}

static void setup()
{
    //ip4_addr_set_u32(&addr, IP_LOOPBACKNET);

    // Have to do a little dance because ipv6 might be present also,
    // so ip_2_ip4 macro is needed to navigate that
    ip4_addr_set_loopback(ip_2_ip4(&loopback_addr));

    //addr.u_addr.ip4 = PP_HTONL(IPADDR_LOOPBACK);
}

static void test_retry_1()
{
    static const char* TAG = "test_retry_1";

    ESP_LOGI(TAG, "entry");

    xTaskCreate(test_retry_1_worker,
        "test retry #1 worker",
        4096,
        nullptr,
        2,
        nullptr);

#if FEATURE_COAP_LWIP_LOOPBACK_TESTS
    // Tried doing a separate pcb_recv to avoid ref == 1 errors, but no
    // dice.  It's more tied to tracker behavior
    embr::lwip::udp::Pcb pcb_recv;

    pcb_recv.alloc();

    ESP_LOGD(TAG, "pcb_recv.has_pcb()=%d", pcb_recv.has_pcb());

    pcb_recv.bind(&loopback_addr, port);
    pcb_recv.recv(udp_resent_receive);

    // Just to ensure loopback has time to get received again.  Not
    // sure if this is actually required.
    // DEBT: Make this into a semaphore
    estd::this_thread::sleep_for(estd::chrono::milliseconds(250));

    pcb_recv.free();
#endif

    ESP_LOGI(TAG, "exit");
}

#endif

#endif

#ifdef ESP_IDF_TESTING
TEST_CASE("retry tests", "[retry]")
#else
void test_retry()
#endif
{
#if FEATURE_COAP_LWIP_LOOPBACK_TESTS
    setup();
#endif

#if __cplusplus >= 201103L
    RUN_TEST(test_retry_1);
#endif
}