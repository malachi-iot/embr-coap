#pragma once

#include "observer-aggregate.h"


namespace embr { namespace coap {

template <class TIncomingContext, class TCollection>
void AggregateDecoderObserver<TIncomingContext, TCollection>::on_token(const pipeline::MemoryChunk::readonly_t& token_part, bool last_chunk)
{

}

template <class TIncomingContext, class TCollection>
void AggregateDecoderObserver<TIncomingContext, TCollection>::on_option(number_t number, uint16_t length)
{

}

template <class TIncomingContext, class TCollection>
void AggregateDecoderObserver<TIncomingContext, TCollection>::on_option(number_t number, const ro_chunk_t& option_value_part, bool last_chunk)
{

}

template <class TIncomingContext, class TCollection>
void AggregateDecoderObserver<TIncomingContext, TCollection>::on_payload(const ro_chunk_t& payload_part, bool last_chunk)
{

}


}}
