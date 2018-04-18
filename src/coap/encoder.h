#pragma once

#include "../coap-encoder.h"

namespace moducom { namespace coap {


// 4/18/2018 - after much experimentation, I have settled on network-buffer pattern
// reigning supreme for shuttling memory around, so then this is the canonical go-to
// encoder
template <class TNetBuf>
class NetBufEncoder : protected experimental::EncoderBase
{
    typedef TNetBuf netbuf_t;

    // outgoing network buffer
    // TODO: for netbuf, modify APIs slightly to be more C++ std lib like, specifically
    // a size/capacity/max_size kind of thing
    netbuf_t m_netbuf;

    OptionEncoder option_encoder;
    typedef Option::Numbers option_number_t;

protected:
    // consolidate netbuf access so as to more easily diagnose incorrect TNetBuf
    // FIX: not cool this const-dropping cast
    uint8_t* data() { return (uint8_t*) m_netbuf.data(); }

    void advance(int amount) { m_netbuf.advance(amount); }

    int size() const { return m_netbuf.length(); }

    // TODO: next should return a tri-state, success, fail, or pending
    bool next() { return m_netbuf.next(); }

    // TODO: make a netbuf-native version of this call, and/or do
    // some extra trickery to ensure chunk() is always efficient
    int max_size() { return m_netbuf.chunk().size; }

    void write(const uint8_t* d, int len)
    {
        memcpy(data(), d, len);
        advance(len);
    }

    template <int N>
    void write(uint8_t (&d) [N])
    {
        write(d, N);
    }

    template <int N>
    void write(const uint8_t (&d) [N])
    {
        write(d, N);
    }

public:
    template <class TNetBufInitParam>
    NetBufEncoder(TNetBufInitParam& netbufinitparam) :
        m_netbuf(netbufinitparam) {}

    bool header(const Header& header)
    {
        assert_state(_state_t::Header);

        // TODO: handle incomplete write
        if(size() >= 4)
        {
            write(header.bytes);
            state(_state_t::HeaderDone);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool token(const moducom::coap::layer2::Token& value)
    {
        assert_state(_state_t::HeaderDone);
        write(value.data(), value.length());
        state(_state_t::TokenDone);
        return true;
    }

    bool option(option_number_t number, const pipeline::MemoryChunk& option_value);

    void option(option_number_t number)
    {
        //option_encoder.option(writer, number);
    }

    // acquire direct access to underlying netbuf, useful for bulk operations like
    // payload writes
    netbuf_t& netbuf()
    {
        return m_netbuf;
    }
};


}}
