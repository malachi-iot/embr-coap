#include "unit-test.h"

#include <exp/lwip/subject.hpp>

using namespace embr::coap;
using namespace embr::coap::experimental;
using namespace embr::coap::experimental::observable;

static void notification_recv(void* arg, struct udp_pcb* _pcb, struct pbuf* p,
    const ip_addr_t* addr, u16_t port)
{
    static const char* TAG = "notification_recv";

    ESP_LOGD(TAG, "entry");

    auto decoder = lwip::Notifier::decoder_factory::create(p);

    Header header = decoder.process_iterate();
}


constexpr static int base_port = 10100,
    client_port = base_port;    // port on which observer client #1 is listening

#ifdef ESP_IDF_TESTING
TEST_CASE("experimental tests", "[experimental]")
#else
void test_experimental()
#endif
{
    static const char* TAG = "test_experimental";

    typedef embr::lwip::internal::Endpoint<false> endpoint_type;
    typedef Registrar<endpoint_type> registrar_type;
    lwip::Notifier notifier;
    typedef lwip::Notifier::encoder_type encoder_type;
    typedef embr::coap::layer2::Token token_type;
    embr::lwip::udp::Pcb pcb_recv(udp_new()), pcb_send(udp_new());

    registrar_type registrar;

    pcb_recv.bind(&loopback_addr, client_port);
    pcb_recv.recv(notification_recv);

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

    // DEBT: We may need a semaphore here, not sure how loopback mode handles
    // that

    pcb_recv.free();
    pcb_send.free();
}
