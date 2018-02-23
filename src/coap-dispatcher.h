//
// Created by Malachi Burke on 12/26/17.
//

#ifndef MC_COAP_TEST_COAP_DISPATCHER_H
#define MC_COAP_TEST_COAP_DISPATCHER_H

#include "coap-decoder.h"
#include "mc/linkedlist.h"

// This adds 8 bytes to IDispatcherHandler class when enabled
//#define FEATURE_IISINTERESTED

// This adds quite a few bytes for an IMessageObserver since each IObserver variant has its own
// vtable pointer
//#define FEATURE_DISCRETE_OBSERVERS

namespace moducom { namespace coap {

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
    typedef experimental::option_number_t number_t;

    virtual void on_header(Header header) = 0;
    virtual void on_token(const pipeline::MemoryChunk::readonly_t& token_part, bool last_chunk) = 0;
    virtual void on_option(number_t number, uint16_t length) = 0;
    virtual void on_option(number_t number, const pipeline::MemoryChunk::readonly_t& option_value_part, bool last_chunk) = 0;
    virtual void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part, bool last_chunk) = 0;
};
#endif

// At this point, only experimental because I keep futzing with the names - don't like IDispatcherHandler
// Perhaps IMessageHandler - finally - is the right term?  But IDispatcher + interested() call *do* go
// together...
namespace experimental {

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

private:
    typedef InterestedEnum interested_t;

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



// Dispatcher/DispatcherHandler
// forms a linked list of handlers who inspect incoming messages and based on a flag set
// one particular dispatcher handler will become the main handler for the remainder of the
// incoming CoAP message

class IDispatcherHandler :
    public moducom::experimental::forward_node<IDispatcherHandler>,
    public IMessageObserver
#ifdef FEATURE_IISINTERESTED
        ,
    public IIsInterested
#endif
{
public:
    virtual ~IDispatcherHandler() {}

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
class DispatcherHandlerBase :
        public IDispatcherHandler,
        public IsInterestedBase
{
protected:
    void interested(InterestedEnum _interested)
    {
        IsInterestedBase::interested(_interested);
    }

    DispatcherHandlerBase(InterestedEnum _interested = Currently)
    {
        interested(_interested);
    }

public:
    virtual InterestedEnum interested() const OVERRIDE
    {
        return IsInterestedBase::interested();
    };

    void on_header(Header header) OVERRIDE { }

    void on_token(const pipeline::MemoryChunk::readonly_t& token_part,
                  bool last_chunk) OVERRIDE
    {}

    void on_option(number_t number, uint16_t length) OVERRIDE
    {

    }

    void on_option(number_t number,
                   const pipeline::MemoryChunk::readonly_t& option_value_part,
                   bool last_chunk) OVERRIDE
    {

    }

    void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                    bool last_chunk) OVERRIDE
    {

    }
};


// Dispatches one CoAP message at a time based on externally provided input
// TODO: Break this out into Decoder base class, right now dispatching and decoding
// are just a little bit too fused together, at least better than IMessageHandler and IMessageObserver
// (IMessageObserver's interest should always be determined externally and IMessageHandler is just
// to generic to say either way)
class Dispatcher :
    public Decoder,
    public moducom::experimental::forward_list<IDispatcherHandler>
{


    // returns false while chunk/context has not been exhausted
    // returns true once it has
    bool dispatch_iterate(Context& context);

    void dispatch_header();
    void dispatch_token();

    // optionChunk is a subset/processed version of dispatch(chunk)
    size_t dispatch_option(const pipeline::MemoryChunk::readonly_t& optionChunk);
    void dispatch_payload(const pipeline::MemoryChunk::readonly_t& payloadChunk, bool last_chunk);

public:
    typedef IDispatcherHandler handler_t;

private:
    void reset()
    {
    }

public:
    // returns number of bytes processed from chunk
    size_t dispatch(const pipeline::MemoryChunk::readonly_t& chunk, bool last_chunk = true)
    {
        Context context(chunk, last_chunk);

        while(!dispatch_iterate(context) && state() != Done);

        return context.pos;
    }

    Dispatcher()
    {
        reset();
    }
};

// An in-place new is expected
typedef IDispatcherHandler* (*dispatcher_handler_factory_fn)(pipeline::MemoryChunk);


/*
struct ShimDispatcherHandlerTraits
{
    static bool evaluate_header() { return true; }
    static bool evaluate_token() { return true; }
    static bool evaluate_payload() { return true; }
}; */


// Splitting this out since we don't always need count embedded in
// it
template <class T>
class ArrayHelperBase
{
    T* items;

public:
    ArrayHelperBase(T* items) : items(items) {}

    inline static void construct(T* items, size_t count)
    {
        while(count--)
        {
            new (items++) T();
        }
    }

    inline static void destruct(T* items, size_t count)
    {
        while(count--)
        {
            // NOTE: I thought one had to call the destructor
            // explicitly , I thought there was no placement
            // delete but here it is...
            delete (items++);
        }
    }

    inline void construct(size_t count) { construct(items, count);}
    inline void destruct(size_t count) { destruct(items, count); }

    operator T* () const { return items; }
};

// Apparently placement-new-array is problematic in that it has a platform
// specific addition of count to the buffer - but no way to ascertain how
// many bytes that count actually uses.  So, use this helper instead
//
// Also does double duty and carries count around in its instance mode
// since so often we need to know that also
template <class T, typename TSize = size_t>
class ArrayHelper : public ArrayHelperBase<T>
{
    const TSize _count;
    typedef ArrayHelperBase<T> base_t;

public:
    ArrayHelper(T* items, TSize count)
            : base_t(items), _count(count) {}

    TSize count() const {return _count;}

    void construct() { base_t::construct(_count);}
    void destruct() { base_t::destruct(_count); }
};

// Utilizes a list of IDispatcherHandler factories to create then
// evaluate incoming messages for interest.  NOTE: Each created
// IDispatcherHandler is immediately destructed if it does not
// express interest, so until one is chosen, it is considered stateless
// this also means that if any option (dispatch-on) data is chunked, it
// will create an awkwardness so likely that needs to be cached up before
// hand, if necessary
//template <class TTraits = ShimDispatcherHandlerTraits>
// Not doing TTraits just yet because that would demand a transition to an .hpp file,
// which I am not against but more work than appropriate right now
//
// Presently this class also elects the first IDispatcherHandler which expresses
// an "Always" interested as the one and only one to further process the message
// clearly this isn't a system-wide desirable behavior, so be warned.  We do this
// because the memory management scheme only supports one truly active IDispatcherHandler
class FactoryDispatcherHandler : public IDispatcherHandler
{
    const dispatcher_handler_factory_fn* handler_factories;
    const int handler_factory_count;

    class State : public IsInterestedBase
    {
        // FIX: optimize this out
        bool state_initialized;

    public:
        bool initialized() const { return state_initialized; }

        void interested(InterestedEnum value)
        {
            state_initialized = true;
            IsInterestedBase::interested(value);
        }

        State() : state_initialized(false) {}
    };

    pipeline::MemoryChunk _handler_memory;


    // Doesn't work yet, 5 unit tests end up not running for some reason
#define FEATURE_FDH_FANCYMEM

#ifdef FEATURE_FDH_FANCYMEM
    // where candidate handlers get their semi-objstack from
    pipeline::MemoryChunk handler_memory() const
    {
        // skip past what we use for handler states
        return _handler_memory.remainder(handler_states_size());
    }

    State* handler_states() const
    {
        return reinterpret_cast<State*>(_handler_memory.data());
    }

    void init_states()
    {
        ArrayHelperBase<State>::construct(handler_states(), handler_factory_count);
    }
#else
    const pipeline::MemoryChunk& handler_memory() const
    {
        return _handler_memory;
    }

    State _handler_states[10];

    State* handler_states() { return _handler_states; }

#endif
    IDispatcherHandler* chosen;

    size_t handler_states_size() const
    {
        return handler_factory_count * sizeof(State);
    }

    State& handler_state(int index) { return handler_states()[index]; }

public:
    FactoryDispatcherHandler(
            const pipeline::MemoryChunk& handler_memory,
            dispatcher_handler_factory_fn* handler_factories,
            int handler_factory_count)

            :_handler_memory(handler_memory),
             handler_factories(handler_factories),
             handler_factory_count(handler_factory_count),
             chosen(NULLPTR)
    {
#ifdef FEATURE_FDH_FANCYMEM
        init_states();
#endif

    }


    virtual void on_header(Header header) OVERRIDE;

    // get called repeatedly until all portion of token is provided
    // Not called if header reports a token length of 0
    virtual void on_token(const pipeline::MemoryChunk::readonly_t& token_part,
                          bool last_chunk) OVERRIDE;

    // gets called once per discovered option, followed optionally by the
    // on_option value portion taking a pipeline message
    virtual void on_option(number_t number, uint16_t length) OVERRIDE;

    // will get called repeatedly until option_value is completely provided
    // Not called if option_header.length() == 0
    virtual void on_option(number_t number,
                           const pipeline::MemoryChunk::readonly_t& option_value_part,
                           bool last_chunk) OVERRIDE;

    virtual void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                            bool last_chunk) OVERRIDE;

    virtual interested_t interested() const OVERRIDE
    {
        return chosen == NULLPTR ?
               IsInterestedBase::Currently : IsInterestedBase::Always;
    }
};



}

}}

#endif //MC_COAP_TEST_COAP_DISPATCHER_H
