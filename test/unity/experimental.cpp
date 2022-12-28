#include "unit-test.h"

#include <exp/lwip/subject.hpp>

using namespace embr::coap;
using namespace embr::coap::experimental;
using namespace embr::coap::experimental::observable;

#ifdef ESP_IDF_TESTING
TEST_CASE("experimental tests", "[experimental]")
#else
void test_experimental()
#endif
{
    typedef embr::lwip::internal::Endpoint<false> endpoint_type;
    typedef Registrar<endpoint_type> registrar_type;
    lwip::Notifier notifier;
    typedef lwip::Notifier::encoder_type encoder_type;
    embr::lwip::udp::Pcb pcb;

    registrar_type registrar;

    // DEBT: Need to loopback on pcb - might even crash, though it's not right now
    notifier.notify(registrar, 0, pcb,
        [](const registrar_type::key_type& key, encoder_type& encoder)
        {
            encoder.option(Option::ContentFormat, Option::TextPlain);
            encoder.payload();
            encoder_type::ostream_type out = encoder.ostream();

            out << "hi2u";
        });
}
