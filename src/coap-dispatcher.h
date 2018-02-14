//
// Created by Malachi Burke on 12/26/17.
//

#ifndef MC_COAP_TEST_COAP_DISPATCHER_H
#define MC_COAP_TEST_COAP_DISPATCHER_H

#include "coap-decoder.h"
#include "mc/linkedlist.h"

namespace moducom { namespace coap {

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


struct IMessageObserver :     public IHeaderObserver,
                              public ITokenObserver,
                              public IOptionObserver,
                              public IPayloadObserver
{

};


// At this point, only experimental because I keep futzing with the names - don't like IDispatcherHandler
// Perhaps IMessageHandler - finally - is the right term?  But IDispatcher + interested() call *do* go
// together...
namespace experimental {

// Dispatcher/DispatcherHandler
// forms a linked list of handlers who inspect incoming messages and based on a flag set
// one particular dispatcher handler will become the main handler for the remainder of the
// incoming CoAP message

class IDispatcherHandler :
    public moducom::experimental::forward_node<IDispatcherHandler>,
    public IMessageObserver
{
public:
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

    virtual ~IDispatcherHandler() {}

    // NOTE: Experimental
    // flags dispatcher to know that this particular handler wants to be the main handler
    // for remainder of CoAP processing
    virtual InterestedEnum interested() = 0;

    inline bool is_interested()
    {
        InterestedEnum i = interested();

        return i == Always || i == Currently;
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
class FactoryDispatcherHandler : public IDispatcherHandler
{
    const dispatcher_handler_factory_fn* handler_factories;
    const int handler_factory_count;

    pipeline::MemoryChunk handler_memory;

    IDispatcherHandler* chosen;

public:
    FactoryDispatcherHandler(
            const pipeline::MemoryChunk& handler_memory,
            dispatcher_handler_factory_fn* handler_factories,
            int handler_factory_count)

            :handler_memory(handler_memory),
             handler_factories(handler_factories),
             handler_factory_count(handler_factory_count),
             chosen(NULLPTR)
    {}


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

    virtual InterestedEnum interested() OVERRIDE
    {
        return Currently;
    }
};



}

}}

#endif //MC_COAP_TEST_COAP_DISPATCHER_H
