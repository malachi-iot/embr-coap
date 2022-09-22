/**
 * 
 * References:
 * 
 * 1. lwip.fandom.com/wiki/Raw/UDP
 */

#include "unit-test.h"

#include <estd/chrono.h>
#include <estd/semaphore.h>

#include <exp/retry/factory.h>
#include <exp/retry.h>

#if __cplusplus >= 201103L
#ifdef ESP_IDF_TESTING
// DEBT: Really we want to test against any LwIP capable target - not just from esp-idf
#define LWIP_PRESENT 1
#include "esp_log.h"
#endif

using namespace embr;


#if defined(ESTD_OS_FREERTOS)
typedef estd::chrono::freertos_clock clock_type;
#else
// DEBT: Not tested in this context
typedef estd::chrono::steady_clock clock_type;
#endif
typedef typename clock_type::time_point time_point;
typedef embr::internal::layer1::Scheduler<8, embr::internal::scheduler::impl::Function<time_point> > scheduler_type;


template <class TStreambuf>
static void setup_outgoing_packet(embr::coap::StreambufEncoder<TStreambuf>& encoder, uint16_t mid = 0x123)
{
    const char* TAG = "setup_outgoing_packet";

    // DEBT: CON Not yet a requirement, but needs to be
    coap::Header header{coap::Header::Confirmable};

    header.message_id(mid);
    
    ESP_LOGV(TAG, "phase 1");

    encoder.header(header);

    ESP_LOGV(TAG, "phase 2");

    // FIX: Currently there seems to be a bug in opbuf_streambuf or encoder itself that expands
    // underlying buffer from 128 to 256 during this operation
    encoder.option(coap::Option::LocationPath, "v1");   // TODO: Can't remember if this is the right URI option
    encoder.option(coap::Option::ContentFormat, coap::Option::ApplicationJson);

    ESP_LOGV(TAG, "phase 3");

    encoder.payload();

    auto o = encoder.ostream();

    o << "{ 'val': 'hi2u' }";

    ESP_LOGV(TAG, "phase 4");

    encoder.finalize();
}

#if LWIP_PRESENT
#include "lwip/tcpip.h"

#include <embr/platform/lwip/endpoint.h>
#include <embr/platform/lwip/pbuf.h>
#include <embr/platform/lwip/istream.h>
#include <embr/platform/lwip/ostream.h>

#include <coap/platform/ip.h>

// Needed for 'finalize' specialization, though that doesn't compile even with this include so far
#include <coap/platform/lwip/encoder.h>

// If LwIP loopback capability is present, then consider enabling our loopback tests
#if LWIP_HAVE_LOOPIF && LWIP_LOOPBACK_MAX_PBUFS
#ifndef FEATURE_COAP_LWIP_LOOPBACK_TESTS
#define FEATURE_COAP_LWIP_LOOPBACK_TESTS 1
#endif
#endif

#if LWIP_TCPIP_CORE_LOCKING != 1
#error Need LWIP_TCPIP_CORE_LOCKING feature
#endif


namespace embr { namespace coap { namespace experimental {

template <>
struct StreambufProvider<embr::lwip::Pbuf>
{
    // NOTE: char or uint8_t are fully functional.  However, ostreambuf of uint8_t
    // has big limitations if using the 'ostream' helper method.
    typedef embr::lwip::upgrading::basic_opbuf_streambuf<char> ostreambuf_type;
    typedef embr::lwip::upgrading::basic_ipbuf_streambuf<uint8_t> istreambuf_type;
};

}}}

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

typedef coap::experimental::retry::Manager<clock_type, transport_type> manager_type;
typedef coap::experimental::EncoderFactory<embr::lwip::Pbuf> encoder_factory;

static ip_addr_t loopback_addr;
constexpr static int base_port = 10000,
    server_port = base_port,    // port to which CON gets sent
    ack_port = base_port + 1;   // port on which to receive ACK
    

static void setup_outgoing_packet(embr::lwip::Pbuf& buffer)
{
    const char* TAG = "setup_outgoing_packet";

    ESP_LOGD(TAG, "entry");

    auto encoder = encoder_factory::create(buffer);

    setup_outgoing_packet(encoder);

    ESP_LOGD(TAG, "exit");
}


// NOTE: Before consolidating into send_ack, we were getting odd behavior where the packet
// seemed to send and be looped back, but udp_ack_receive never picked it up.  Furthermore,
// that was accompanied by a ping going over loopback.  Never determined why that happened.
static void send_ack(embr::lwip::udp::Pcb& pcb, embr::coap::Header incoming_header)
{
    static const char* TAG = "send_ack";

    ESP_LOGI(TAG, "entry: header.mid=%x", incoming_header.message_id());

    // In this instance, auto-allocating Pbuf is perfect
    embr::lwip::Pbuf pbuf(4);   // Header is 4 bytes
    embr::coap::Header& header = incoming_header;

    header.type(embr::coap::Header::Acknowledgement);

    // Copy header into outgoing pbuf
    // DEBT: consider omitting 'len' since it's expected to match buf->tot_len
    pbuf.take(&header, 4);

    err_t result = pcb.send(pbuf, &loopback_addr, ack_port);

    ESP_LOGV(TAG, "sent: result=%d", result);
}

#if defined(ESTD_OS_FREERTOS)
static estd::freertos::counting_semaphore<1, true>
    signal1,
    signal2;

volatile bool end_signaled = false;
volatile bool ack_received = false;


// loopback:port+1 destination
static void udp_ack_receive(void* arg, struct udp_pcb* _pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
    static const char* TAG = "udp_ack_receive";

    // NOTE: As expected, this misbehaves because it's not in the main app
    // thread.  So, queuing up asserted variables instead
    //TEST_ASSERT_NULL(arg);

    auto& manager = * (manager_type*) arg;

    ESP_LOGI(TAG, "entry. receive from %s:%u, manager.empty()=%u",
        ipaddr_ntoa(addr),
        port,
        manager.tracker.empty());

    embr::lwip::udp::Pcb pcb(_pcb);
    embr::lwip::Pbuf pbuf(p);

    endpoint_type
        // endpoint = from where we received this ACK
        endpoint(addr, port),
        // reconstructed = what we expect endpoint to be
        endpoint_reconstructed(&loopback_addr, port);

    embr::coap::Header header = embr::coap::experimental::get_header(pbuf);

    const auto& item = **manager.tracker.begin();

    ESP_LOGV(TAG, "mid=%x, type=%s, pre-match=%u",
        header.message_id(),
        embr::coap::get_description(header.type()),
        endpoint == endpoint_reconstructed);

    ESP_LOGD(TAG, "tracked endpoint=%s:%u, mid=%x",
        ipaddr_ntoa(item.endpoint().address()),
        item.endpoint().port(),
        item.mid());

    ack_received = manager.on_received(endpoint, pbuf);

    ESP_LOGD(TAG, "ack_received=%d", ack_received);

    // DEBT: Do direct task notifications, since we tightly control all the tasks
    signal1.release();

    ESP_LOGI(TAG, "exit");
}



// loopback:port destination
static void udp_resent_receive(void* arg, struct udp_pcb* _pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
    static const char* TAG = "udp_resent_receive";

    ESP_LOGI(TAG, "entry: receive from port %u", port);

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


static void test_retry_1_worker(void* parameter)
{
    static const char* TAG = "test_retry_1_worker";

    ESP_LOGI(TAG, "entry");

    embr::lwip::udp::Pcb pcb;
    embr::lwip::Pbuf buffer(128);
    auto raw_pbuf = buffer.pbuf();  // using because std::move nulls out buffer

    setup_outgoing_packet(buffer);

    pcb.alloc();

    ESP_LOGD(TAG, "pcb.has_pcb()=%d", pcb.has_pcb());

    scheduler_type scheduler;
    endpoint_type server_endpoint(&loopback_addr, server_port);

    // DEBT: Add copy/move constructor to TransportUdp
    //transport_type transport(pcb);

    manager_type manager(pcb);

    LOCK_TCPIP_CORE();  // Just experimenting, this lock/unlock doesn't seem to help
    pcb.bind(&loopback_addr, ack_port);
    //pcb.bind(IP_ADDR_ANY, port + 1);  // Doesn't make a difference
    pcb.recv(udp_ack_receive, &manager);
    UNLOCK_TCPIP_CORE();

    //pbuf_ref(buffer);

    //ESP_LOGD(TAG, "ref=%d, tot_len=%d", buffer.pbuf()->ref, buffer.total_length());
    ESP_LOGD(TAG, "ref=%d, tot_len=%d, payload=%p, next=%p",
        raw_pbuf->ref, raw_pbuf->tot_len, raw_pbuf->payload, raw_pbuf->next);

    // [1] indicates udp_send doesn't free the thing.  Also, udp.c source code
    // indicates "p is still referenced by the caller, and will live on"

    // DEBT: This really needs to be put into TransportUdp
    LOCK_TCPIP_CORE();
    
    manager.send(server_endpoint, time_point(estd::chrono::seconds(5)),
        std::move(buffer), scheduler);

    // FIX: udp_send modifies pbuf. tot_len and payload are changed to
    // accomodate header.  So far this botches our resend mechanism
    // https://stackoverflow.com/questions/68786784/is-it-possible-to-use-udp-sendto-to-resend-an-udp-packet
    ESP_LOGD(TAG, "ref=%d, tot_len=%d, payload=%p, next=%p",
        raw_pbuf->ref, raw_pbuf->tot_len, raw_pbuf->payload, raw_pbuf->next);

    // DEBT: Consider adding semaphore here to make ack inspector wait
    // for send to completely finish

    UNLOCK_TCPIP_CORE();

    ESP_LOGD(TAG, "data packet sent, now waiting.  manager.empty()=%u",
        manager.tracker.empty());

    end_signaled = signal1.try_acquire_for(estd::chrono::milliseconds(1000));

    pcb.free();

    ESP_LOGI(TAG, "exit");

    signal2.release();

    vTaskDelete(NULL);
}


static void test_retry_1()
{
    static const char* TAG = "test_retry_1";

    ESP_LOGI(TAG, "entry");

#if FEATURE_COAP_LWIP_LOOPBACK_TESTS
    embr::experimental::Unique<embr::lwip::udp::Pcb> pcb_recv;

    ESP_LOGD(TAG, "pcb_recv.has_pcb()=%d", pcb_recv.has_pcb());

    pcb_recv.bind(&loopback_addr, server_port);
    pcb_recv.recv(udp_resent_receive);

    xTaskCreate(test_retry_1_worker,
        "retry worker",
        4096,
        nullptr,
        2,
        nullptr);

    // Just to ensure loopback has time to get received again.
    signal2.try_acquire_for(estd::chrono::milliseconds(1000));

    TEST_ASSERT_TRUE(end_signaled);

    // FIX: This one fails.  Perhaps endpoint == is broken?
    TEST_ASSERT_TRUE(ack_received);
#endif

    ESP_LOGI(TAG, "exit");
}

#endif // FREERTOS

static void setup()
{
#if FEATURE_COAP_LWIP_LOOPBACK_TESTS
    ip_addr_set_loopback(false, &loopback_addr);    // IPv4 loopback
#endif
}

#else   // LWIP_PRESENT

// TODO: Create transport_type and endpoint_type not dependent on LwIP

#endif  // LWIP_PRESENT

typedef coap::experimental::retry::Tracker<time_point, transport_type> tracker_type;

static void test_tracker()
{
    tracker_type tracker;
    time_point zero_time;
    typedef typename transport_type::buffer_type buffer_type;
    typedef typename tracker_type::item_type item_type;
    constexpr uint16_t mid = 0x4321;

    // DEBT: Decouple from LwIP
    endpoint_type endpoint(&loopback_addr, server_port);
    buffer_type buffer(128);
    auto encoder = encoder_factory::create(buffer);
    setup_outgoing_packet(encoder, mid);

    TEST_ASSERT_TRUE(tracker.empty());

    const item_type* item = tracker.track(endpoint, zero_time, std::move(buffer));

    TEST_ASSERT_FALSE(tracker.empty());
    TEST_ASSERT_EQUAL(mid, item->mid());

    auto it = tracker.match(endpoint, mid);

    TEST_ASSERT_TRUE(it == tracker.begin());
    TEST_ASSERT_FALSE(it == tracker.end());

    tracker.untrack(it);

    TEST_ASSERT_TRUE(tracker.empty());
}



#endif  // c++11


#ifdef ESP_IDF_TESTING
TEST_CASE("retry tests", "[retry]")
#else
void test_retry()
#endif
{
    setup();

#if __cplusplus >= 201103L
    RUN_TEST(test_tracker);
    RUN_TEST(test_retry_1);
#endif
}