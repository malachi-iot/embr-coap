#pragma once

#include <estd/optional.h>

#include <coap/internal/rfc7641/notifier.hpp>
#include <coap/platform/lwip/rfc7641/notifier.hpp>

typedef embr::coap::internal::observable::sequence_type sequence_type;

typedef embr::lwip::internal::Endpoint<false> endpoint_type;

namespace paths {

enum
{
    v1 = 0,
    v1_api,
    v1_api_gpio,
    v1_api_version,
    v1_api_stats,
    v1_api_gpio_value,

    well_known,
    well_known_core
};


}

namespace internal {

// DEBT: Since underlying lwip::Notifier (and similar) are pretty fused with this,
// change internal::NotifyHelper to regular Notifier and change underyling lwip::Notifier
// to impl
template <class TTransport, class TRegistrar>
struct NotifyHelper;

namespace codes {
// General purpose error codes
// DEBT: Was hoping to avoid something like this, though it's not SO bad
enum CoapStatus
{
    OK = 0,
    Full                ///<! capacity reached for underlying container
};

}



template <class TRegistrar>
struct NotifyHelper<embr::lwip::udp::Pcb, TRegistrar> :
    embr::coap::internal::NotifyHelperBase<TRegistrar>
{
    typedef embr::coap::internal::NotifyHelperBase<TRegistrar> base_type;
    typedef typename base_type::registrar_type registrar_type;

    embr::lwip::udp::Pcb pcb;

    // DEBT: This is shaping up to be an impl
    typedef embr::coap::internal::observable::lwip::Notifier notifier_type;

    NotifyHelper(embr::lwip::udp::Pcb pcb) :
        pcb(pcb)
    {
    }

    NotifyHelper(embr::lwip::udp::Pcb pcb, registrar_type& registrar) :
        pcb(pcb),
        base_type(registrar)
    {
    }

    template <typename F>
    void notify(typename registrar_type::handle_type handle, F&& f)
    {
        notifier_type::notify(base_type::registrar, handle, pcb, std::move(f));
    }
};

}

typedef internal::NotifyHelper<
    embr::lwip::udp::Pcb,
    embr::coap::internal::observable::layer1::Registrar<endpoint_type, 10> >
    NotifyHelper;

typedef NotifyHelper::notifier_type::encoder_type encoder_type;
typedef NotifyHelper::registrar_type registrar_type;

// NOTE: Only builds option and payload portion
void build_stat(encoder_type& encoder, sequence_type sequence = sequence_type());


extern NotifyHelper* notifier;