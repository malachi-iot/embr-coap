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



namespace experimental {

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
    TEncoder& encoder() const { return *_encoder; }

    // TODO: signal that encoding is done (probably to datapump, but
    // not necessarily)
    void encoding_complete() {}
};


class OutputContext
{
public:
    template <class TNetBuf>
    void send(TNetBuf& to_send) {}
};


template <class DataPump, DataPump* global_datapump>
class GlobalDatapumpOutputContext
{
    typedef typename DataPump::netbuf_t netbuf_t;
    typedef typename DataPump::addr_t addr_t;

public:
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
    void send(netbuf_t&& out, const addr_t& addr_out)
    {
        global_datapump->enqueue_out(std::forward<netbuf_t>(out), addr_out);
    }
#else
    void send(netbuf_t& out, const addr_t& addr_out)
    {
        global_datapump->enqueue_out(out, addr_out);
    }
#endif
};


// For RFC 7641
// TODO: Once promoted from experimental, move this elsewhere
namespace observable {

// Designed to fit into a 2 bit value
enum Options
{
    Register = 0,
    Deregister = 1,

    Sequence = 2,   // EXPERIMENTAL

    Unspecified = 3,
};


// DEBT: Doesn't fit naming convention, and kinda sloppy in general
typedef estd::layer1::optional<Options, Unspecified> option_value_type;
typedef estd::layer1::optional<uint32_t, 0x1000000> sequence_type;


}

}


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
template <bool inline_token, bool simple_buffer = false>
struct TokenAndHeaderContext;

template <>
struct TokenAndHeaderContext<true, false> :
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


template <>
struct TokenAndHeaderContext<false, false> :
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



// New-generation request context, replacement for premature coap_transmission one
// Incoming request handlers/dispatchers may extend this context, but this is a good
// foundational base class
// Called Incoming context because remember some incoming messages are RESPONSES, even
// when we are the server (ACKs, etc)
template <class TAddr, bool inline_token = true, bool simple_buffer = false>
class IncomingContext :
        public TokenAndHeaderContext<inline_token, simple_buffer>,
        public AddressContext<TAddr>
{
public:
    IncomingContext() {}

    IncomingContext(const TAddr& addr) :
        AddressContext<TAddr>(addr)
    {}
};


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

/*
 * Disabled during mc-mem removal
struct ObjStackContext
{
    // NOTE: Facing a small cunundrum: objstack doesn't know during a free operation
    // how many bytes to free, and generic dispatch handlers such as FactoryDispatcherHandler
    // and AggregateUriPathObserver don't know how many bytes their children are using
    moducom::dynamic::ObjStack objstack;

    ObjStackContext(const moducom::pipeline::MemoryChunk& chunk) : objstack(chunk) {}
};

// FIX: Seems more appropriate that ObserverContext and IncomingContext
// would be one and the same, but for now in a semi-experimental fashion
// keep them separate
struct ObserverContext :
        public IncomingContext<uint8_t[4]>,
        public ObjStackContext
{
    ObserverContext(const moducom::pipeline::MemoryChunk& chunk) : ObjStackContext(chunk) {}
};
*/

template <class TContext>
struct incoming_context_traits
{
    typedef typename TContext::addr_t addr_t;

    template <class TAddr>
    static void set_address(TContext& c, const TAddr& addr)
    {
        //c.address(addr);
    }
};

// message observer support code
template <class Context, class TContextTraits >
class ContextContainer
{
protected:
    Context* m_context;

public:
    typedef Context context_t;
    typedef TContextTraits context_traits_t;

    context_t& context() { return *m_context; }

    const context_t& context() const { return *m_context; }

    // FIX: Kludgey way of skipping some steps.  Strongly consider
    // dumping this if we can
    void context(context_t& context)
    {
        this->m_context = &context;
    }

};


namespace internal {

// DEBT: Poor naming and organization
// This is where utility bitmask goes + auto reply info
class ExtraContext
{
public:
    // For auto response
    estd::layer1::optional<Header::Code::Codes, Header::Code::Empty> response_code;
    
    struct
    {
        // duplicate MID encountered
        bool dup_mid : 1;
        // payload encountered
        bool payload : 1;
        // whether response has yet been sent or is queued for send
        bool response_sent : 1;

        // observe option, if present
        // NOTE: does not retain sequence number
        experimental::observable::Options observable : 2;

    }   flags;

    ExtraContext()
    {
        flags.dup_mid = false;
        flags.payload = false;
        flags.response_sent = false;
        flags.observable = experimental::observable::option_value_type::null_value();
    }

    experimental::observable::option_value_type observe_option() const
    {
        return flags.observable;
    }
};

}


}}
