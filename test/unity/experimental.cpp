#include <esp_log.h>

#include "unit-test.h"

#include <embr/observer.h>
// DEBT: A little mysterious how to know to include this particular
// hpp for decode_and_notify
#include <coap/decoder/subject-core.hpp>
#include <coap/decoder/observer/util.h>

#include <exp/lwip/subject.hpp>

using namespace embr::coap;
using namespace embr::coap::internal;
using namespace embr::coap::internal::observable;


// Not yet used
struct NotificationApp
{
    static constexpr const char* TAG = "NotificationApp";

    void on_notify(event::header e)
    {

    }

    void on_notify(event::token e)
    {
        
    }
};

static void notification_recv(void* arg, struct udp_pcb* _pcb, struct pbuf* p,
    const ip_addr_t* addr, u16_t port)
{
    static const char* TAG = "notification_recv";
    const char* name = (const char*) arg;

    ESP_LOGD(TAG, "entry: %s", name);

    auto decoder = lwip::Notifier::decoder_factory::create(p);

    // DEBT: 'false' (non inline) is more appropriate for this test, but looks like
    // it needs work
    TokenAndHeaderContext<true> context;

    embr::layer1::subject<
        HeaderContextObserver,
        TokenContextObserver,
        NotificationApp> s;

    decode_and_notify(decoder, s, context);
}


constexpr static int base_port = 10100,
    client_port = base_port,    // port on which observer client #1 is listening
    client2_port = base_port + 1;

typedef embr::lwip::internal::Endpoint<false> endpoint_type;
typedef observable::layer1::Registrar<endpoint_type, 10> registrar_type;

static embr::lwip::udp::Pcb pcb_recv(udp_new()), pcb_send(udp_new());

static void test_notifier()
{
    static const char* TAG = "test_notifier";

    lwip::Notifier notifier;
    typedef lwip::Notifier::encoder_type encoder_type;
    typedef embr::coap::layer2::Token token_type;
    embr::lwip::udp::Pcb pcb_client2;

    pcb_client2.alloc();

    registrar_type registrar;

    pcb_recv.bind(&loopback_addr, client_port);
    pcb_recv.recv(notification_recv, (void*)"client 1");
    pcb_client2.bind(&loopback_addr, client2_port);
    pcb_client2.recv(notification_recv, (void*)"client 2");

    registrar_type::key_type client(
        endpoint_type(&loopback_addr, client_port),
        token_type((uint8_t*)"hi2u", 4));

    registrar.add(client, 0);

    // DEBT: Need to loopback on pcb - might even crash, though it's not right now
    notifier.notify(registrar, 0, pcb_send,
        [](const registrar_type::key_type& key, encoder_type& encoder)
        {
            ESP_LOGD(TAG, "notify");

            encoder.option(Option::ContentFormat, Option::TextPlain);
            encoder.payload();
            encoder_type::ostream_type out = encoder.ostream();

            out << "hi2u";
        });
    
    // DEBT: May need a semaphore or similar, depending on how lwip handles
    // loopback

    pcb_client2.free();
}

#ifdef ESP_IDF_TESTING
TEST_CASE("experimental tests", "[experimental]")
#else
void test_experimental()
#endif
{
    RUN_TEST(test_notifier);

    pcb_recv.free();
    pcb_send.free();
}
