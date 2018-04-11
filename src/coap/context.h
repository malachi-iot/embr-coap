//
// Created by malachi on 2/25/18.
//

#pragma once

#include "../coap-token.h"
#include "../coap-header.h"
#include "../coap-features.h"

// TODO:
// a) add decoder state accessor/decoder* to context itself for convenient query as to
//    present state of decode
namespace moducom { namespace coap {


namespace experimental {

template <class TAddr>
class AddressContext
{
    TAddr addr;

public:
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
    inline const pipeline::MemoryChunk::readonly_t token() const
    {
        size_t tkl = header().token_length();

        // Have to create an inline chunk since our native token has no length
        // and a pointer would clearly violate stack rules
        // Considering strongly using an inline layer2::Token but that would be a shame
        // since we'd be tracking token length in two places - but at this point, no
        // less efficient than _token_present flag
        // Separately, considering instead using a decoder::state() pointer which would take
        // an extra byte but could provide a lot of utility - such as more accurate assertion
        // of a valid header
        return _token.subset(tkl);
    }

    void token(const pipeline::MemoryChunk::readonly_t* t)
    {
        TokenContext::token(t);
    }
#endif
};

}}
