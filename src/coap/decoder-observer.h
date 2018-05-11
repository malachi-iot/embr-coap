#pragma once

namespace moducom { namespace coap {



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

    virtual void on_token(const pipeline::MemoryChunk::readonly_t& token_part,
                  bool last_chunk) OVERRIDE
    {}

    virtual void on_option(number_t number, uint16_t length) OVERRIDE
    {

    }

    virtual void on_option(number_t number,
                   const pipeline::MemoryChunk::readonly_t& option_value_part,
                   bool last_chunk) OVERRIDE
    {

    }

    virtual void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                    bool last_chunk) OVERRIDE
    {

    }

#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
    virtual void on_complete() OVERRIDE {}
#endif
};

}}
