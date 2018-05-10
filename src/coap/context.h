//
// Created by malachi on 2/25/18.
//

#pragma once

#include "../coap-token.h"
#include "../coap-header.h"
#include "../coap-features.h"
#include "mc/objstack.h"

// TODO:
// a) add decoder state accessor/decoder* to context itself for convenient query as to
//    present state of decode
namespace moducom { namespace coap {


namespace experimental {

template <class TAddr>
class AddressContext
{
protected:
    TAddr addr;

public:
    typedef TAddr addr_t;

    const TAddr& address() const { return addr; }
};

}

class TokenContext
{
#ifdef FEATURE_MCCOAP_INLINE_TOKEN
protected:
    typedef layer1::Token token_t;

    token_t _token;

    // TODO: kludgey and inefficient - improve this
    bool _token_present;
#else
    typedef layer2::Token token_t;

    const token_t* _token;
#endif

public:
    TokenContext()
#ifdef FEATURE_MCCOAP_INLINE_TOKEN
        : _token_present(false)
#else
        : _token(NULLPTR)
#endif
    {}

#ifdef FEATURE_MCCOAP_INLINE_TOKEN
public:
    void token(const pipeline::MemoryChunk::readonly_t* t)
    {
        _token_present = true;
        _token.copy_from(*t);
    }

    // TODO: Reconcile naming, either call this have_token() or rename
    // have_header to header_present()
    inline bool token_present() const { return _token_present; }
#else
    inline bool token_present() const { return _token; }

    // if a) incoming message has a token and b) we've decoded the token and have it
    // available, then this will be non-null
    const token_t* token() const { return _token; }

    // Since tokens arrive piecemeal and oftentimes not at all, we have a non-constructor
    // based setter
    void token(const token_t* token) { _token = token; }
#endif
};


class HeaderContext
{
    Header _header;

public:
    HeaderContext()
    {
        // always zero out, signifying UNINITIALIZED so that querying
        // parties don't mistakenly think we have a header
        _header.raw = 0;
    }

    // if incoming message has had its header decoded, then this is valid
    const Header& header() const { return _header; }

    void header(const Header& header) { _header.raw = header.raw; }

    bool have_header() const { return _header.version() > 0; }
};



// NOTE: Just an interesting idea at this time
class DecoderContext
{
};


// NOTE: Really would like this (avoid the whole global_encoder nastiness) but
// unclear right now how to incorporate this across different styles of encoder.
// So it remains fully experimental and unused at this time
template <class TEncoder>
class EncoderContext
{
    TEncoder* _encoder;

public:
    TEncoder* encoder() const { return _encoder; }
};

// New-generation request context, replacement for premature coap_transmission one
// Incoming request handlers/dispatchers may extend this context, but this is a good
// foundational base class
// Called Incoming context because remember some incoming messages are RESPONSES, even
// when we are the server (ACKs, etc)
class IncomingContext :
        public TokenContext,
        public HeaderContext,
        // Really dislike this AddressContext, but we do need some way to pass
        // down source-address information so that Observable can look at it
        public experimental::AddressContext<uint8_t[4]>
{
public:
#ifdef FEATURE_MCCOAP_INLINE_TOKEN
    inline const layer2::Token token() const
    {
        return layer2::Token(_token, header().token_length());
    }

    void token(const pipeline::MemoryChunk::readonly_t* t)
    {
        TokenContext::token(t);
    }
#endif
};


// FIX: Seems more appropriate that ObserverContext and IncomingContext
// would be one and the same, but for now in a semi-experimental fashion
// keep them separate
struct ObserverContext : public IncomingContext
{
    // NOTE: Facing a small cunundrum: objstack doesn't know during a free operation
    // how many bytes to free, and generic dispatch handlers such as FactoryDispatcherHandler
    // and AggregateUriPathObserver don't know how many bytes their children are using
    dynamic::ObjStack objstack;

    ObserverContext(const pipeline::MemoryChunk& chunk) : objstack(chunk) {}
};

}}
