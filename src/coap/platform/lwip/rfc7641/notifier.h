#pragma once

#include "../../../internal/rfc7641/keys.h"
#include "../context.h"   // for EncoderFactory and friends
#include "../encoder.h"   // For encoder finalize specialization

#include <embr/platform/lwip/endpoint.h>

namespace embr { namespace coap {

namespace internal { namespace observable { namespace lwip {

class Notifier
{
public:
    typedef embr::lwip::internal::Endpoint<false> endpoint_type;
    typedef RegistrarKeyBase::handle_type handle_type;
    typedef coap::experimental::EncoderFactory<
        struct pbuf*, embr::coap::experimental::LwipPbufFactory<64> > encoder_factory;
    typedef coap::experimental::DecoderFactory<struct pbuf*> decoder_factory;
    typedef typename encoder_factory::encoder_type encoder_type;

    template <typename TContainer, typename F>
    static void notify(detail::Registrar<TContainer>& registrar, handle_type handle,
        embr::lwip::udp::Pcb pcb, F&& f);
};

}

template <class TRegistrar>
struct Notifier<embr::lwip::udp::Pcb, TRegistrar> :
    embr::coap::internal::NotifyHelperBase<TRegistrar>
{
    typedef embr::coap::internal::NotifyHelperBase<TRegistrar> base_type;
    typedef typename base_type::registrar_type registrar_type;

    embr::lwip::udp::Pcb pcb;

    // DEBT: This is shaping up to be an impl
    typedef embr::coap::internal::observable::lwip::Notifier notifier_type;

    Notifier(embr::lwip::udp::Pcb pcb) :
        pcb(pcb)
    {
    }

    Notifier(embr::lwip::udp::Pcb pcb, registrar_type& registrar) :
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

}}


}}