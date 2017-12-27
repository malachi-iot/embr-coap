//
// Created by Malachi Burke on 12/26/17.
//

#ifndef MC_COAP_TEST_COAP_DISPATCHER_H
#define MC_COAP_TEST_COAP_DISPATCHER_H

#include "coap-decoder.h"
#include "mc/linkedlist.h"

namespace moducom { namespace coap {

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


template <class TState>
class StateHelper
{
    TState _state;

protected:
    void state(TState _state) { this->_state = _state; }

    StateHelper(TState _state) { state(_state); }

public:
    StateHelper() {}

    TState state() const { return _state; }
};

class DispatcherBase
{
public:
    // Copy/paste from old "Parser" class
    enum State
    {
        Uninitialized,
        Header,
        HeaderDone,
        Token,
        TokenDone,
        OptionsStart,
        Options,
        OptionsDone, // all options are done being processed
        Payload,
        PayloadDone,
        // Denotes completion of entire CoAP message
        Done,
    };

    typedef State state_t;
};

// Dispatches one CoAP message at a time based on externally provided input
class Dispatcher :
    public DispatcherBase,
    public moducom::experimental::forward_list<IDispatcherHandler>,
    public StateHelper<DispatcherBase::state_t>
{
    struct Context
    {
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

    // TODO: Union-ize this  Not doing so now because of C++03 trickiness
    HeaderDecoder headerDecoder;
    OptionDecoder optionDecoder;
    OptionDecoder::OptionExperimental optionHolder;
    TokenDecoder tokenDecoder;

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

    Dispatcher() : StateHelper(Uninitialized)
    {
        reset();
    }
};

}

}}

#endif //MC_COAP_TEST_COAP_DISPATCHER_H
