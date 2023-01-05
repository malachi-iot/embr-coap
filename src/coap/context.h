//
// Created by malachi on 2/25/18.
//

#pragma once

#include "token.h"
#include "header.h"
#include "../coap-features.h"
//#include "mc/objstack.h"
#include <estd/optional.h>
#include <estd/type_traits.h>

#include <estd/span.h>

#include "internal/context.h"

// TODO:
// a) add decoder state accessor/decoder* to context itself for convenient query as to
//    present state of decode
// b) utilize c++17 (I think) 'concepts' to enforce context signature
namespace embr { namespace coap {

namespace tags {

// DEBT: Fake stand in for c++20 concept.  These tags promise presence of certain methods
// document specifics for each tag
struct token_context {};
struct header_context {};
struct address_context {};
struct incoming_context {};

}

template <class TAddr>
class AddressContext : public tags::address_context
{
protected:
    TAddr addr;

    AddressContext() {}

    AddressContext(const TAddr& addr) : addr(addr) {}

public:
    typedef TAddr addr_t;

    const TAddr& address() const { return addr; }
};

// Holds only the data portion of the token, need header.tkl to
// find its length
template <bool inline_token>
class TokenContext;

// inline flavor
template <>
class TokenContext<true> : public tags::token_context
{
protected:
    typedef layer1::Token token_t;

    token_t _token;

    // TODO: kludgey and inefficient - improve this
    bool _token_present;

public:
    TokenContext()
        : _token_present(false)
    {}

public:
    void token(const estd::span<const uint8_t>& t)
    {
        _token_present = true;
        std::copy(t.begin(), t.end(), _token.begin());
        //_token.copy_from(*t);
    }

    // TODO: Reconcile naming, either call this have_token() or rename
    // have_header to header_present()
    inline bool token_present() const { return _token_present; }
};


// NOTE: Code has signficantly changed and this is now considered not tested
// DEBT: Replace this entirely with SimpleBufferContext
template <>
class TokenContext<false> : public tags::token_context
{
protected:
    const uint8_t* _token;

public:
    TokenContext()
        : _token(NULLPTR)
    {}

    // Since tokens arrive piecemeal and oftentimes not at all, we have a non-constructor
    // based setter
    void token(const estd::span<const uint8_t>& token)
    {
        //printf("%02x %02x %02x %02x", token[0], token[1], token[2], token[3]);
#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
        std::clog << std::hex <<  ' ' << (int)token[0];
        std::clog << ' ' << (int) token[1];
        std::clog << ' ' << (int) token[2];
        std::clog << ' ' << (int) token[3];
        std::clog << std::dec << std::endl;
#endif
        _token = token.data();
    }

    // TODO: Reconcile naming, either call this have_token() or rename
    // have_header to header_present()
    inline bool token_present() const { return _token != NULLPTR; }
};



class HeaderContext : public tags::header_context
{
    Header m_header;

public:
    HeaderContext()
    {
        // always zero out, signifying UNINITIALIZED so that querying
        // parties don't mistakenly think we have a header
        m_header.raw = 0;
    }

    // if incoming message has had its header decoded, then this is valid
    const Header& header() const { return m_header; }

    void header(const Header& header) { m_header.raw = header.raw; }

    bool have_header() const { return m_header.version() > 0; }
};





// for scenarios when we definitely don't care about chunking
// and memory scope and still want header/token in the context.
// be mindful to validate this data before tossing it in here
class SimpleBufferContext
{
    internal::const_buffer chunk;

public:
    const Header& header() const
    {
        // FIX: This is viable but see if there's a better way to
        // initialize a const-new scenario
        void* h = (void*)chunk.data();
        return * new (h) Header();
    }

    bool have_header() const { return header().version() > 0; }

    inline layer3::Token token() const
    {
        // skip header portion
        return layer3::Token(chunk.data() + 4, header().token_length());
    }

    inline bool token_present() const { return true; }
};

// these two really like to go together
template <bool inline_>
struct TokenAndHeaderContext;

template <>
struct TokenAndHeaderContext<true> :
        public TokenContext<true>,
        public HeaderContext
{
    inline const layer3::Token token() const
    {
        return layer3::Token(_token.data(),
                             header().token_length());
    }

    void token(const internal::const_buffer& t)
    {
        TokenContext<true>::token(t);
    }
};


// DEBT: Use SimpleBufferContext approach here instead
template <>
struct TokenAndHeaderContext<false> :
        public TokenContext<false>,
        public HeaderContext
{
    inline layer3::Token token() const
    {
        return layer3::Token(_token, header().token_length());
    }

    void token(const internal::const_buffer &t)
    {
        TokenContext<false>::token(t);
    }
};


// Incoming request handlers/dispatchers may extend this context, but this is a good
// foundational base class
// Called Incoming context because remember some incoming messages are RESPONSES, even
// when we are the server (ACKs, etc)
template <class TAddr, bool inline_ = true, bool extra_ = true>
class IncomingContext;


template <class TAddr, bool inline_>
class IncomingContext<TAddr, inline_, false> :
    public TokenAndHeaderContext<inline_>,
    public AddressContext<TAddr>,
    public tags::incoming_context
{
public:
    ESTD_CPP_DEFAULT_CTOR(IncomingContext)

    IncomingContext(const TAddr& addr) :
        AddressContext<TAddr>(addr)
    {}

    // Noop since no ExtraContext is here to track the send
    void on_send() {}
};


template <class TAddr, bool inline_>
class IncomingContext<TAddr, inline_, true> :
    public TokenAndHeaderContext<inline_>,
    public AddressContext<TAddr>,
    public internal::ExtraContext,
    public tags::incoming_context
{
public:
    ESTD_CPP_DEFAULT_CTOR(IncomingContext)

    IncomingContext(const TAddr& addr) :
        AddressContext<TAddr>(addr)
    {}
};


// DEBT: Obsolete, but early experimental retry code still picks it up
template <class TNetBuf>
class NetBufDecoder;


template <class TNetBufDecoder>
struct DecoderContext
{
    typedef TNetBufDecoder decoder_t;
    typedef typename estd::remove_reference<typename decoder_t::netbuf_t>::type netbuf_t;

private:
    decoder_t m_decoder;

#ifdef UNIT_TESTING
public:
#else
protected:
#endif
    DecoderContext(netbuf_t& netbuf) : m_decoder(netbuf) {}

public:
    decoder_t& decoder() { return m_decoder; }
};


}}
