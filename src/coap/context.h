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
// b) utilize c++17 (I think) 'concepts' to enforce context signature
namespace moducom { namespace coap {


template <class TAddr>
class AddressContext
{
protected:
    TAddr addr;

public:
    typedef TAddr addr_t;

    const TAddr& address() const { return addr; }
};

template <bool inline_token>
class TokenContext;

// inline flavor
template <>
class TokenContext<true>
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
    void token(const pipeline::MemoryChunk::readonly_t* t)
    {
        _token_present = true;
        _token.copy_from(*t);
    }

    // TODO: Reconcile naming, either call this have_token() or rename
    // have_header to header_present()
    inline bool token_present() const { return _token_present; }
};


template <>
class TokenContext<false>
{
    typedef layer2::Token token_t;

protected:
    token_t _token;

public:
    TokenContext()
        : _token(NULLPTR, 0)
    {}

    inline bool token_present() const { return _token.length() > 0; }

    // if a) incoming message has a token and b) we've decoded the token and have it
    // available, then this will be non-null
    const token_t& token() const { return _token; }

    // Since tokens arrive piecemeal and oftentimes not at all, we have a non-constructor
    // based setter
    void token(const token_t* token) { _token = *token; }
};



class HeaderContext
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
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
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

}


// for scenarios when we definitely don't care about chunking
// and memory scope and still want header/token in the context.
// be mindful to validate this data before tossing it in here
class SimpleBufferContext
{
    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;

    ro_chunk_t chunk;

public:
    const Header& header() const
    {
        // FIX: This is viable but see if there's a better way to
        // initialize a const-new scenario
        void* h = (void*)chunk.data();
        return * new (h) Header();
    }

    bool have_header() const { return header().version() > 0; }

    inline ro_chunk_t token() const
    {
        return ro_chunk_t(chunk.data(4), header().token_length());
    }
};

// these two really like to go together
template <bool inline_token>
struct TokenAndHeaderContext;

template <>
struct TokenAndHeaderContext<true> :
        public TokenContext<true>,
        public HeaderContext
{
    inline const layer2::Token token() const
    {
        return layer2::Token(_token, header().token_length());
    }

    void token(const pipeline::MemoryChunk::readonly_t* t)
    {
        TokenContext<true>::token(t);
    }
};


template <>
struct TokenAndHeaderContext<false> :
        public TokenContext<false>,
        public HeaderContext {};



// New-generation request context, replacement for premature coap_transmission one
// Incoming request handlers/dispatchers may extend this context, but this is a good
// foundational base class
// Called Incoming context because remember some incoming messages are RESPONSES, even
// when we are the server (ACKs, etc)
template <class TAddr, bool inline_token = true>
class IncomingContext :
        public TokenAndHeaderContext<inline_token>,
        public AddressContext<TAddr>
{
public:
};

struct ObjStackContext
{
    // NOTE: Facing a small cunundrum: objstack doesn't know during a free operation
    // how many bytes to free, and generic dispatch handlers such as FactoryDispatcherHandler
    // and AggregateUriPathObserver don't know how many bytes their children are using
    dynamic::ObjStack objstack;

    ObjStackContext(const pipeline::MemoryChunk& chunk) : objstack(chunk) {}
};


// FIX: Seems more appropriate that ObserverContext and IncomingContext
// would be one and the same, but for now in a semi-experimental fashion
// keep them separate
struct ObserverContext :
        public IncomingContext<uint8_t[4]>,
        public ObjStackContext
{
    ObserverContext(const pipeline::MemoryChunk& chunk) : ObjStackContext(chunk) {}
};


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


}}
