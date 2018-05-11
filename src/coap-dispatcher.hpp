#pragma once

namespace moducom { namespace coap {

namespace experimental {

template <class TRequestContext>
void ContextDispatcherHandler<TRequestContext>::on_header(Header header)
{
    this->context().header(header);
}


template <class TRequestContext>
void ContextDispatcherHandler<TRequestContext>::on_token(const pipeline::MemoryChunk::readonly_t& token_part, bool last_chunk)
{
#ifdef FEATURE_MCCOAP_INLINE_TOKEN
    // NOTE: this won't yet work with chunked
    this->context().token(&token_part);
#else
    // FIX: access already-allocated token manager (aka token pool) and allocate a new
    // token or link to already-allocated token.
    // For now we use very unreliable static to hold this particular incoming token
    static layer2::Token token;

    // NOTE: this won't yet work with chunked
    token.set(token_part.data(), token_part.length());
    context().token(&token);

    // NOTE: Perhaps we don't want to use token pool at this time, and instead keep a full
    // token reference within TokenContext instead.  We can still pool it later in more of
    // a session-token pool area
#endif
}


}

}}
