#pragma once

#include "../coap-encoder.h"
#include "mc/netbuf.h"
#include "../coap-uint.h"

#include <utility>

namespace moducom { namespace coap {




// 4/18/2018 - after much experimentation, I have settled on network-buffer pattern
// reigning supreme for shuttling memory around, so then this is the canonical go-to
// encoder
template <class TNetBuf>
class NetBufEncoder :
        public moducom::io::experimental::NetBufWriter<TNetBuf>,
        protected experimental::EncoderBase
{
    typedef TNetBuf netbuf_t;
    typedef moducom::io::experimental::NetBufWriter<TNetBuf> base_t;
    typedef const pipeline::MemoryChunk::readonly_t ro_chunk_t;

    OptionEncoder option_encoder;

public:
    typedef Option::Numbers option_number_t;
    typedef typename base_t::size_type size_type;

    // acquire direct access to underlying netbuf, useful for bulk operations like
    // payload writes
    netbuf_t& netbuf()
    {
        return this->m_netbuf;
    }

protected:
    size_type write(const pipeline::MemoryChunk::readonly_t& chunk)
    {
        return base_t::write(chunk.data(), chunk.length());
    }

    // experimental
    size_type write(unsigned long val)
    {
        return 0;
    }

    // would use declval to deduce a .data() and .length() provider or not but that's
    // a C++ only thing, so probably gotta retrofit it into estdlib
    template <class TString>
    size_type write(const TString (&s))
    {
        return base_t::write(s);
    }

    // process all NON-value portion of option
    bool option_header(option_number_t number, uint16_t value_length);

    // process ONLY value portion of option
    template <class TMemory>
    bool option_value(const TMemory& chunk, bool last_chunk)
    {
        size_type written = write(chunk);
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
        return m_payload_marker_written;
#endif
    }

    void payload_marker_written(bool is_written)
    {
#ifndef DEBUG
        m_payload_marker_written = is_written;
#endif
    }

public:
    // public so that zero copy operations can kick off payloads properly
    bool payload_header()
    {
        assert_not_state(_state_t::Header);
        assert_not_state(_state_t::Token);
        assert_not_state(_state_t::Payload);
        assert_state(_state_t::OptionsDone);

        if(!payload_marker_written())
        {
            if(this->size() == 0)
            {
                written(0);
                return false;
            }

            this->putchar(COAP_PAYLOAD_MARKER);

            payload_marker_written(true);
        }

        state(_state_t::Payload);

        return true;
    }

    // ensures payload marker is written and then returns current netbuf unprocessed
    // chunk ptr
    uint8_t* payload()
    {
        payload_header();
        return base_t::data();
    }

    size_type write(const void* d, int len)
    {
        return base_t::write(d, len);
    }

protected:
    // represents how many bytes were written during last public/high level encode operation
    // only available if encode operation comes back as false.  Undefined when operation
    // succeeds
    size_type m_written;

    void written(size_type w) { m_written = w; }

public:
    template <class TNetBufInitParam>
    NetBufEncoder(TNetBufInitParam& netbufinitparam) :
        base_t(netbufinitparam)
    {
        payload_marker_written(false);
    }


    NetBufEncoder() { payload_marker_written(false); }

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
        size_type written = base_t::write(data, tkl);
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

    bool option(option_number_t number,
                ro_chunk_t& option_value,
                bool last_chunk = true);

    // have to explicitly do this, otherwise the string-capable version greedily
    // eats things up
    bool option(option_number_t number,
                const pipeline::MemoryChunk& option_value,
                bool last_chunk = true)
    {
        ro_chunk_t& ov = option_value;

        return option(number, ov, last_chunk);
    }

    // FIX: clumsy but helpful.  Once we get SFINAE working right to
    // properly select underlying write operation, things will be cleaner
    bool option(option_number_t number, int option_value)
    {
        coap::layer2::UInt<> v;

        v.set(option_value);

        return option(number, pipeline::MemoryChunk(v));
    }

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

    bool payload(ro_chunk_t option_value, bool last_chunk = true);

    // marks end of encoding - oftentimes, encoder cannot reasonably figure this out due to optional
    // presence of both options and payload.  Only during presence of payload can we
    // indicate a 'last chunk'.  To avoid mixing and matching complete-detection techniques,
    // an explicit call to complete() is always required to signal to underlying netbuf
    // (or perhaps alternate signalling mechanism?) - At time of this writing no formalized
    // signaling mechanism is present
    void complete() { }
};


}}
