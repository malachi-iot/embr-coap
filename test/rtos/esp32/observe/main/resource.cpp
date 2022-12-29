#include "esp_log.h"

#include <embr/observer.h>

#include <coap/platform/lwip/encoder.h>
#include <coap/decoder.hpp>
#include <coap/decoder/events.h>
#include <coap/decoder/subject-core.hpp>

#include <exp/lwip/subject.hpp>

using namespace embr::coap;

typedef embr::lwip::internal::Endpoint<false> endpoint_type;

experimental::observable::Registrar<endpoint_type> registrar;

static experimental::observable::lwip::Notifier notifier;
static embr::lwip::udp::Pcb notifying_pcb;
