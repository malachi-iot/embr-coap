//
// Created by Malachi Burke on 12/26/17.
//

#ifndef MC_COAP_TEST_COAP_DISPATCHER_H
#define MC_COAP_TEST_COAP_DISPATCHER_H

#include "coap-decoder.h"
#include "mc/linkedlist.h"

namespace moducom { namespace coap {

// re-write and decomposition of IResponderDeprecated
struct IHeaderInput
{
    virtual void on_header(CoAP::Header header) = 0;
};


struct ITokenInput
{
    // get called repeatedly until all portion of token is provided
    // Not called if header reports a token length of 0
    virtual void on_token(const pipeline::MemoryChunk& token_part, bool last_chunk) = 0;
};

struct IOptionInput
{
    typedef CoAP::OptionExperimentalDeprecated::Numbers number_t;

    // gets called once per discovered option, followed optionally by the
    // on_option value portion taking a pipeline message
    virtual void on_option(number_t number, uint16_t length) = 0;

    // will get called repeatedly until option_value is completely provided
    // Not called if option_header.length() == 0
    virtual void on_option(number_t number, const pipeline::MemoryChunk& option_value_part, bool last_chunk) = 0;
};


struct IPayloadInput
{
    // will get called repeatedly until payload is completely provided
    // IMPORTANT: if no payload is present, then payload_part is nullptr
    // this call ultimately marks the end of the coap message (unless I missed something
    // for chunked/multipart coap messages... which I may have)
    virtual void on_payload(const pipeline::MemoryChunk& payload_part, bool last_chunk) = 0;
};


namespace experimental {

// Dispatcher/DispatcherHandler
// forms a linked list of handlers who inspect incoming messages and based on a flag set
// one particular dispatcher handler will become the main handler for the remainder of the
// incoming CoAP message

class IDispatcherHandler :
    public moducom::experimental::forward_node<IDispatcherHandler>,
    public IHeaderInput,
    public ITokenInput,
    public IOptionInput,
    public IPayloadInput
{
public:
    enum InterestedEnum
    {
        Always,
        Currently,
        NotRightNow,
        Never
    };
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
// are just a little bit too fused together
class Dispatcher :
    public Decoder,
    public moducom::experimental::forward_list<IDispatcherHandler>
{
    struct Context
    {
        // TODO: optimize by making this a value not a ref, and bump up "data" pointer
        // (and down length) instead of bumping up pos.  A little more fiddly, but then
        // we less frequently have to create new temporary memorychunks on the stack

        const pipeline::MemoryChunk& chunk;

        // current processing position.  Should be chunk.length once processing is done
        size_t pos;

        // flag which indicates this is the last chunk to be processed for this message
        // does NOT indicate if a boundary demarkates the end of the coap message BEFORE
        // the chunk itself end
        bool last_chunk;

        // Unused helper function
        const uint8_t* data() const { return chunk.data + pos; }

    public:
        Context(const pipeline::MemoryChunk& chunk, bool last_chunk) :
                chunk(chunk),
                last_chunk(last_chunk),
                pos(0) {}
    };

    // returns false while chunk/context has not been exhausted
    // returns true once it has
    bool dispatch_iterate(Context& context);

    void dispatch_header();
    void dispatch_token();

    // optionChunk is a subset/processed version of dispatch(chunk)
    size_t dispatch_option(const pipeline::MemoryChunk& optionChunk);
    void dispatch_payload(const pipeline::MemoryChunk& payloadChunk, bool last_chunk);

public:
    typedef IDispatcherHandler handler_t;

private:
    void reset()
    {
    }

public:
    // returns number of bytes processed from chunk
    size_t dispatch(const pipeline::MemoryChunk& chunk, bool last_chunk = true)
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

}

}}

#endif //MC_COAP_TEST_COAP_DISPATCHER_H
