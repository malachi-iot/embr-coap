#pragma once

#include "../coap-encoder.h"

namespace moducom { namespace coap {


// a thinner wrapper around netbuf, mainly adding convenience methods for
// writes
template <class TNetBuf>
class NetBufWriter
{
protected:
    typedef TNetBuf netbuf_t;

    // outgoing network buffer
    // TODO: for netbuf, modify APIs slightly to be more C++ std lib like, specifically
    // a size/capacity/max_size kind of thing
    netbuf_t m_netbuf;

public:
    template <class TNetBufInitParam>
    NetBufWriter(TNetBufInitParam& netbufinitparam) :
            m_netbuf(netbufinitparam)
    {}

    // acquire direct access to underlying netbuf, useful for bulk operations like
    // payload writes
    netbuf_t& netbuf() const
    {
        return this->m_netbuf;
    }

    // this represents TNetBuf size_type.  Can't easily pull this out of netbuf directly
    // due to the possibility TNetBuf might be a reference (will instead have to do fancy
    // C++11 decltype/declval things to deduce it)
    typedef int size_type;

    bool advance(size_type amount)
    {
        // TODO: add true/false on advance to underlying netbuf itself to aid in runtime
        // detection of boundary failure
        netbuf().advance(amount);
        return true;
    }

    size_type size() const { return netbuf().length(); }


    size_type write(const uint8_t* d, int len)
    {
        if(len > size()) len = size();

        // FIX: ugly, netbuf itself really needs to drop const, at least when
        // it's a writeable netbuf
        memcpy((void*)netbuf().data(), d, len);
        bool advance_success = advance(len);

        ASSERT_ERROR(true, advance_success, "Problem advancing through netbuf");

        // TODO: decide if we want to issue a next() call here or externally
        return len;
    }

    template <class TString>
    size_type write(TString s)
    {
        int copied = s.copy((char*)netbuf().data(), this->size());

        this->advance(copied);

        ASSERT_ERROR(false, copied > s.length(), "Somehow copied more than was available!");

        return copied;
    }

};

// 4/18/2018 - after much experimentation, I have settled on network-buffer pattern
// reigning supreme for shuttling memory around, so then this is the canonical go-to
// encoder
template <class TNetBuf>
class NetBufEncoder :
        public NetBufWriter<TNetBuf>,
        protected experimental::EncoderBase
{
    typedef TNetBuf netbuf_t;
    typedef NetBufWriter<TNetBuf> base_t;

    OptionEncoder option_encoder;

public:
    typedef Option::Numbers option_number_t;
    // this represents TNetBuf size_type.  Can't easily pull this out of netbuf directly
    // due to the possibility TNetBuf might be a reference (will instead have to do fancy
    // C++11 decltype/declval things to deduce it)
    typedef int size_type;

    // acquire direct access to underlying netbuf, useful for bulk operations like
    // payload writes
    netbuf_t& netbuf() const
    {
        return this->m_netbuf;
    }

protected:
    // consolidated netbuf access so as to more easily diagnose incorrect TNetBuf
    // FIX: not cool this const-dropping cast
    uint8_t* data() { return (uint8_t*) netbuf().data(); }

    // TODO: next should return a tri-state, success, fail, or pending
    bool next() { return netbuf().next(); }

    // TODO: make a netbuf-native version of this call, and/or do
    // some extra trickery to ensure chunk() is always efficient
    size_type max_size() { return netbuf().chunk().size; }

    size_type write(const pipeline::MemoryChunk::readonly_t& chunk)
    {
        return base_t::write(chunk.data(), chunk.length());
    }

    template <int N>
    size_type write(uint8_t (&d) [N])
    {
        return base_t::write(d, N);
    }

    template <int N>
    size_type write(const uint8_t (&d) [N])
    {
        return base_t::write(d, N);
    }

    // process all NON-value portion of option
    bool option_header(option_number_t number, uint16_t value_length);

    // process ONLY value portion of option
    bool option_value(pipeline::MemoryChunk chunk, bool last_chunk)
    {
        size_type written = base_t::write(chunk.data(), chunk.length());
        if(written == chunk.length())
        {
            if (last_chunk)
                option_encoder.fast_forward();

            return true;
        }
        else
        {
            this->written(written);
            return false;
        }
    }

#ifndef DEBUG
    // NOTE: having this presents a strong case to *always* do state/consistency
    // feature, not just debug mode
    bool m_payload_marker_written;
#endif

    bool payload_marker_written() const
    {
#ifdef DEBUG
        return state() == _state_t::Payload;
#else
#endif
    }

    void payload_marker_written(bool is_written)
    {
#ifndef DEBUG
        m_payload_marker_written = is_written;
#endif
    }

    bool payload_header()
    {
        assert_not_state(_state_t::Header);
        assert_not_state(_state_t::Token);
        assert_not_state(_state_t::Payload);

        if(!payload_marker_written())
        {
            if(this->size() == 0) return false;

            data()[0] = 0xFF;
            this->advance(1);
            payload_marker_written(true);
        }

        state(_state_t::Payload);

        return true;
    }

    // represents how many bytes were written during last public/high level encode operation
    // only available if encode operation comes back as false.  Undefined when operation
    // succeeds
    size_type m_written;

    void written(size_type w) { m_written = w; }

public:
    template <class TNetBufInitParam>
    NetBufEncoder(TNetBufInitParam& netbufinitparam) :
        NetBufWriter<TNetBuf>(netbufinitparam)
    {
        payload_marker_written(false);
    }

    size_type written() const { return m_written; }

    bool header(const Header& header)
    {
        assert_state(_state_t::Header);

        // TODO: handle incomplete write
        if(this->size() >= 4)
        {
            write(header.bytes);
            state(_state_t::HeaderDone);
            return true;
        }
        else
        {
            written(0);
            return false;
        }
    }


    // this variety does not handle chunking on the input
    bool token(const uint8_t* data, size_type tkl)
    {
        assert_state(_state_t::HeaderDone);
        size_type written = write(data, tkl);
        state(_state_t::TokenDone);
        this->written(written);
        return written == tkl;
    }

    bool token(const pipeline::MemoryChunk::readonly_t& value, bool last_chunk = true)
    {
        // TODO: handle chunking
        assert_state(_state_t::HeaderDone);
        size_type written = write(value);
        state(_state_t::TokenDone);
        this->written(written);
        return written == value.length();
    }


    /*
    bool token(const moducom::coap::layer2::Token& value)
    {
        assert_state(_state_t::HeaderDone);
        write(value.data(), value.length());
        state(_state_t::TokenDone);
        return true;
    } */

    bool option(option_number_t number, const pipeline::MemoryChunk& option_value, bool last_chunk = true);

    // TString should match std::string signature
    template <class TString>
    bool option(option_number_t number, TString s);

    bool option(option_number_t number)
    {
        return option_header(number, 0);
    }


    // NOTE: this has no provision for chunking, this is a one-shot payload string
    // assignment
    template <class TString>
    bool payload(TString s);

    bool payload(const pipeline::MemoryChunk& option_value, bool last_chunk = true);

    // marks end of encoding - oftentimes, encoder cannot reasonably figure this out due to optional
    // presence of both options and payload.  Only during presence of payload can we
    // indicate a 'last chunk'.  To avoid mixing and matching complete-detection techniques,
    // an explicit call to complete() is always required to signal to underlying netbuf
    // (or perhaps alternate signalling mechanism?) - At time of this writing no formalized
    // signaling mechanism is present
    void complete() {}
};


}}
