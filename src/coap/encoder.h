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
    // this represents TNetBuf size_type
    typedef int size_type;

protected:
    // consolidate netbuf access so as to more easily diagnose incorrect TNetBuf
    // FIX: not cool this const-dropping cast
    uint8_t* data() { return (uint8_t*) m_netbuf.data(); }

    void advance(size_type amount) { m_netbuf.advance(amount); }

    size_type size() const { return m_netbuf.length(); }

    // TODO: next should return a tri-state, success, fail, or pending
    bool next() { return m_netbuf.next(); }

    // TODO: make a netbuf-native version of this call, and/or do
    // some extra trickery to ensure chunk() is always efficient
    size_type max_size() { return m_netbuf.chunk().size; }

    size_type write(const uint8_t* d, int len)
    {
        if(len > size()) len = size();

        memcpy(data(), d, len);
        advance(len);

        // TODO: decide if we want to issue a next() call here or externally
        return len;
    }

    template <int N>
    size_type write(uint8_t (&d) [N])
    {
        return write(d, N);
    }

    template <int N>
    size_type write(const uint8_t (&d) [N])
    {
        return write(d, N);
    }

    // process all NON-value portion of option
    bool option_header(option_number_t number, uint16_t value_length);

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

    // TString should match std::string signature
    template <class TString>
    bool option(option_number_t number, TString s);

    bool option(option_number_t number)
    {
        return option_header(number, 0);
    }

    // acquire direct access to underlying netbuf, useful for bulk operations like
    // payload writes
    netbuf_t& netbuf()
    {
        return m_netbuf;
    }
};


}}
