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

};


template <class TState>
class StateHelper
{
    TState _state;

protected:
    void state(TState _state) { this->_state = _state; }

public:
    TState state() const { return _state; }
};

class DispatcherBase
{
public:
    // Copy/paste from old "Parser" class
    enum State
    {
        Header,
        HeaderDone,
        Token,
        TokenDone,
        OptionsStart,
        Options,
        OptionsDone, // all options are done being processed
        Payload
    };

    typedef State state_t;
};

// Dispatches one CoAP message at a time based on externally provided input
class Dispatcher :
    public moducom::experimental::forward_list<IDispatcherHandler>,
    public StateHelper<DispatcherBase::state_t>
{
public:
    //Dispatcher()

};

}

}}

#endif //MC_COAP_TEST_COAP_DISPATCHER_H
