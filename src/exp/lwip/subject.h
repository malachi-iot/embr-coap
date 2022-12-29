#pragma once

#include "../subject.h"
#include "../../coap/platform/lwip/context.h"   // for EncoderFactory and friends
#include "../../coap/platform/lwip/encoder.h"   // For encoder finalize specialization
#include <embr/platform/lwip/endpoint.h>

namespace embr { namespace coap {

namespace experimental { namespace observable { namespace lwip {

class Notifier
{
public:
    typedef embr::lwip::internal::Endpoint<false> endpoint_type;
    typedef RegistrarKeyBase::handle_type handle_type;
    typedef EncoderFactory<
        struct pbuf*, embr::coap::experimental::LwipPbufFactory<64> > encoder_factory;
    typedef DecoderFactory<struct pbuf*> decoder_factory;
    typedef typename encoder_factory::encoder_type encoder_type;

    template <typename TContainer, typename F>
    static void notify(detail::Registrar<TContainer>& registrar, handle_type handle,
        embr::lwip::udp::Pcb pcb, F&& f);
};

}}}

}}