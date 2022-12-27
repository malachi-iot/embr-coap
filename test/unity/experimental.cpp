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
    /*
    typedef embr::lwip::internal::Endpoint<false> endpoint_type;
    typedef Registrar<endpoint_type> registrar_type;
    lwip::Notifier notifier;

    registrar_type registrar;

    notifier.notify(registrar, 0, []{}); */
}
