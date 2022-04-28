#pragma once

#include "observer.h"

namespace embr { namespace coap {

// this is runtime-ish collection version
template <class TIncomingContext, class TCollection>
class AggregateDecoderObserver : public DecoderObserverBase<TIncomingContext>
{
    TCollection observers;
    typedef typename TCollection::iterator iterator;
    typedef DecoderObserverBase<TIncomingContext> base_t;
    typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;
    typedef typename base_t::number_t number_t;

public:
    template <class TCollectionInitParam>
    AggregateDecoderObserver(TCollectionInitParam param) : observers(param)
    {

    }

    virtual void on_header(Header header) OVERRIDE
    {
        iterator i = observers.begin();
    }

    virtual void on_token(const moducom::pipeline::MemoryChunk::readonly_t& token_part, bool last_chunk) OVERRIDE;
    virtual void on_option(number_t number, uint16_t length) OVERRIDE;
    virtual void on_option(number_t number, const ro_chunk_t& option_value_part, bool last_chunk) OVERRIDE;
    virtual void on_payload(const ro_chunk_t& payload_part, bool last_chunk) OVERRIDE;
};

}}
