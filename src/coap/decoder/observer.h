// 12/03/2019: OBSOLETE
// We now use embr subject-observer mechanism, this is not compatible
#pragma once

#include <mc/memory-pool.h>
#include "../context.h"

namespace embr { namespace coap {

#ifdef FEATURE_DISCRETE_OBSERVERS
// re-write and decomposition of IResponderDeprecated
struct IHeaderObserver
{
    virtual void on_header(Header header) = 0;
};


struct ITokenObserver
{
    // get called repeatedly until all portion of token is provided
    // Not called if header reports a token length of 0
    virtual void on_token(const pipeline::MemoryChunk::readonly_t& token_part, bool last_chunk) = 0;
};

struct IOptionObserver
{
    typedef experimental::option_number_t number_t;

    // gets called once per discovered option, followed optionally by the
    // on_option value portion taking a pipeline message
    virtual void on_option(number_t number, uint16_t length) = 0;

    // will get called repeatedly until option_value is completely provided
    // Not called if option_header.length() == 0
    virtual void on_option(number_t number, const pipeline::MemoryChunk::readonly_t& option_value_part, bool last_chunk) = 0;
};


struct IPayloadObserver
{
    // will get called repeatedly until payload is completely provided
    // IMPORTANT: if no payload is present, then payload_part is nullptr
    // this call ultimately marks the end of the coap message (unless I missed something
    // for chunked/multipart coap messages... which I may have)
    virtual void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part, bool last_chunk) = 0;
};



struct IOptionAndPayloadObserver :
        public IOptionObserver,
        public IPayloadObserver
{};


struct IMessageObserver :   public IHeaderObserver,
                            public ITokenObserver,
                            public IOptionAndPayloadObserver
{

};
#else
struct IMessageObserver
{
    typedef internal::option_number_t number_t;

    virtual void on_header(Header header) = 0;
    virtual void on_token(const moducom::pipeline::MemoryChunk::readonly_t& token_part, bool last_chunk) = 0;
    virtual void on_option(number_t number, uint16_t length) = 0;
    virtual void on_option(number_t number, const moducom::pipeline::MemoryChunk::readonly_t& option_value_part, bool last_chunk) = 0;
    virtual void on_payload(const moducom::pipeline::MemoryChunk::readonly_t& payload_part, bool last_chunk) = 0;
#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
    virtual void on_complete() = 0;
#endif
};
#endif

// FIX: Name not quite right, this is the explicitly NON virtual/polymorphic flavor
// *AND* contains convenience backing field for interested()
struct IsInterestedBase
{
    // Enum representing interest level for one particular message
    // interest level for multiple messages is undefined
    enum InterestedEnum
    {
        // This dispatcher is always interested in receiving observer messages
        Always,
        // This dispatcher is currently interested (aka curious/possibly) but
        // may not be in the future
        Currently,
        // This dispatcher is currently NOT interested, but may become interested
        // in the future
        NotRightNow,
        // This dispatcher never wants any more observations
        Never
    };

    typedef InterestedEnum interested_t;

private:

    interested_t _interested;

protected:
    void interested(interested_t _interested)
    {
        this->_interested = _interested;
    }


public:
    inline interested_t interested() const
    {
        return _interested;
    }

    inline bool is_always_interested() const
    {
        return _interested == Always;
    }


    inline bool is_never_interested() const
    {
        return _interested == Never;
    }


    inline static bool is_interested(interested_t value)
    {
        return value == Always ||
               value == Currently;
    }

    bool is_interested() const { return is_interested(_interested); }
};

struct IIsInterested
{
    typedef IsInterestedBase::InterestedEnum interested_t;

    // NOTE: Experimental
    // flags dispatcher to know that this particular handler wants to be the main handler
    // for remainder of CoAP processing
    virtual interested_t interested() const = 0;

    inline bool is_interested() const
    { return IsInterestedBase::is_interested(interested()); }


    inline bool is_always_interested() const
    {
        return interested() == IsInterestedBase::Always;
    }
};


template <class TIncomingContext = ObserverContext,
          class TRequestContextTraits = incoming_context_traits<TIncomingContext> >
class IDecoderObserver :
    public IMessageObserver
#ifdef FEATURE_IISINTERESTED
        ,
    public IIsInterested
#endif
{
public:
    typedef TIncomingContext context_t;

    virtual ~IDecoderObserver() {}

    // set this to a particular context
    virtual void context(context_t& context) = 0;

#ifndef FEATURE_IISINTERESTED
    typedef IsInterestedBase::InterestedEnum interested_t;

    // NOTE: Experimental
    // flags dispatcher to know that this particular handler wants to be the main handler
    // for remainder of CoAP processing
    virtual interested_t interested() const = 0;

    inline bool is_interested() const
    { return IsInterestedBase::is_interested(interested()); }


    inline bool is_always_interested() const
    {
        return interested() == IsInterestedBase::Always;
    }
#endif
};


// Convenience class for building dispatcher handlers
template <class TIncomingContext = ObserverContext,
          class TIncomingContextTraits = incoming_context_traits<TIncomingContext> >
class DecoderObserverBase :
        public IDecoderObserver<TIncomingContext, TIncomingContextTraits>,
        public IsInterestedBase,
        public ContextContainer<TIncomingContext, TIncomingContextTraits>
{
protected:
    typedef IDecoderObserver<TIncomingContext> base_t;
    typedef ContextContainer<TIncomingContext, TIncomingContextTraits> ccontainer_t;
    typedef typename ccontainer_t::context_t context_t;
    typedef typename base_t::number_t number_t;

#ifndef FEATURE_IISINTERESTED
    inline bool is_always_interested() const
    {
        // ensure we pick up the one that utilizes virtual interested()
        return base_t::is_always_interested();
    }
#endif

    void interested(InterestedEnum _interested)
    {
        IsInterestedBase::interested(_interested);
    }

    DecoderObserverBase(InterestedEnum _interested = Currently)
    {
        interested(_interested);
    }

public:
    virtual void context(context_t& c) OVERRIDE
    {
        ccontainer_t::context(c);
    }

    context_t& context() { return ccontainer_t::context(); }

    const context_t& context() const { return ccontainer_t::context(); }

    virtual InterestedEnum interested() const OVERRIDE
    {
        return IsInterestedBase::interested();
    };

    void on_header(Header header) OVERRIDE { }

    virtual void on_token(const moducom::pipeline::MemoryChunk::readonly_t& token_part,
                  bool last_chunk) OVERRIDE
    {}

    virtual void on_option(number_t number, uint16_t length) OVERRIDE
    {

    }

    virtual void on_option(number_t number,
                   const moducom::pipeline::MemoryChunk::readonly_t& option_value_part,
                   bool last_chunk) OVERRIDE
    {

    }

    virtual void on_payload(const moducom::pipeline::MemoryChunk::readonly_t& payload_part,
                    bool last_chunk) OVERRIDE
    {

    }

#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
    virtual void on_complete() OVERRIDE {}
#endif
};

}}
