#pragma once

#include "../../coap.h" // more resilient against other coap.h's when we do this

namespace moducom { namespace coap {

// TODO: Needs a better home than simple.h
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

/*!
 * Simply counts down number of bytes as a "decode" operation
 */
template <typename TCounter = size_t>
class CounterDecoder
{
    TCounter pos;

protected:
    TCounter position() const { return pos; }

public:
    CounterDecoder() : pos(0) {}

    // returns true when we achieve our desired count
    inline bool process_iterate(TCounter max_size)
    {
        return ++pos >= max_size;
    }

    void reset() { pos = 0; }
};


template <size_t buffer_size, typename TCounter=uint8_t>
class RawDecoder : public CounterDecoder<TCounter>
{
    uint8_t buffer[buffer_size];

public:
    typedef CounterDecoder<TCounter> base_t;

    const uint8_t* data() const { return buffer; }

    // returns true when all bytes finally accounted for
    inline bool process_iterate(uint8_t value, TCounter max_size)
    {
        buffer[base_t::position()] = value;
        return base_t::process_iterate(max_size);
    }
};

typedef RawDecoder<8> TokenDecoder;


class HeaderDecoder :
    public Header,
    public CounterDecoder<uint8_t>
{
    typedef CounterDecoder<uint8_t> base_t;

public:
    // returns true when complete
    inline bool process_iterate(uint8_t value)
    {
        bytes[base_t::position()] = value;
        return base_t::process_iterate(4);
    }
};

}}
