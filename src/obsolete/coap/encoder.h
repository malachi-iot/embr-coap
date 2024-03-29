#pragma once

#include <coap/encoder/option.h>
#include <coap-encoder.h>
#include "mc/netbuf.h"
#include <coap/uint.h>
#include <coap/context.h>

#include <estd/span.h>
#include <estd/utility.h>

namespace embr { namespace coap {


namespace pipeline = moducom::pipeline;
namespace dynamic = moducom::dynamic;


// 4/18/2018 - after much experimentation, I have settled on network-buffer pattern
// reigning supreme for shuttling memory around, so then this is the canonical go-to
// encoder
// 12/8/2019 - no longer exactly the case.  StreambufEncoder is now the preferred mechanism,
// however we're going to keep netbuf based things around in a minimal sense since pre c++11
// code I expect can work with it.  Be advised this code expects a mk. 1 netbuf, so to be
// functional we'll need to upgrade to a mk. 2 netbuf
template <class TNetBuf>
class NetBufEncoder :
        public moducom::io::experimental::NetBufWriter<TNetBuf>,
        protected experimental::EncoderBase
{
    typedef TNetBuf netbuf_t;
    typedef moducom::io::experimental::NetBufWriter<TNetBuf> base_t;
    typedef const estd::const_buffer ro_chunk_t;

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
    size_type write(const estd::const_buffer& chunk)
    {
        return base_t::write(chunk.data(), chunk.size());
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
    template <class TContainer>
    bool option_value(const TContainer& chunk, bool last_chunk)
    {
        size_type written = write(chunk);
        if(written == chunk.size())
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


    ///
    /// \brief output payload marker
    ///
    /// better name than payload header... header you have to infer the meaning, but marker
    /// is the actual proper name of the 0xFF
    ///
    /// \return
    ///
    bool payload_marker() { return payload_header(); }

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

    template <class TImpl>
    size_type write(const estd::internal::allocated_array<TImpl>& value)
    {
        size_type written = base_t::write(value.clock(), value.size());
        value.cunlock();
        return written;
    }

protected:
    // represents how many bytes were written during last public/high level encode operation
    // only available if encode operation comes back as false.  Undefined when operation
    // succeeds
    size_type m_written;

    void written(size_type w) { m_written = w; }

public:
#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    NetBufEncoder(TArgs&&...args) :
        base_t(std::forward<TArgs>(args)...)
    {
        payload_marker_written(false);
    }
#else
    template <class TNetBufInitParam>
    NetBufEncoder(TNetBufInitParam& netbufinitparam) :
        base_t(netbufinitparam)
    {
        payload_marker_written(false);
    }
#endif


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

    template <class TImpl>
    bool token(const estd::internal::allocated_array<TImpl>& value, bool last_chunk = true)
    {
        // TODO: handle chunking
        assert_state(_state_t::HeaderDone);
        size_type written = write(value);
        state(_state_t::TokenDone);
        this->written(written);
        return written == value.size();
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


    bool token(const estd::const_buffer& b)
    {
        return token(b.data(), b.size());
    }


    bool option(option_number_t number,
                ro_chunk_t& option_value,
                bool last_chunk = true);

    // have to explicitly do this, otherwise the string-capable version greedily
    // eats things up
    bool option(option_number_t number,
                const pipeline::MemoryChunk& option_value,
                bool last_chunk = true)
    {
        // since there's a tiny bit of overload-complexity, do a legacy behavior here
        // until we clean the overload complexity up
        ro_chunk_t ov(option_value.data(), option_value.length());

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

    // do a most-used case for filling out a response based on a requesting
    // token and header context
    template <bool inline_token, bool simple_buffer>
    bool header_and_token(TokenAndHeaderContext<inline_token, simple_buffer>& context,
                          Header::Code::Codes response_code)
    {
        // Not going to check header write, we always assume we have at least 4 bytes
        // at the start of our netbuf
        header(embr::coap::create_response(context.header(),
                                       response_code));
        return token(context.token());
    }

    // marks end of encoding - oftentimes, encoder cannot reasonably figure this out due to optional
    // presence of both options and payload.  Only during presence of payload can we
    // indicate a 'last chunk'.  To avoid mixing and matching complete-detection techniques,
    // an explicit call to complete() is always required to signal to underlying netbuf
    // (or perhaps alternate signalling mechanism?) - At time of this writing no formalized
    // signaling mechanism is present
    void complete()
    {
        //netbuf().complete();
    }
};


}}
