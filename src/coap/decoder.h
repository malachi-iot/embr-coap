#pragma once

#include "../coap-decoder.h"
#include "../coap-token.h"
#include <estd/string.h>

namespace moducom { namespace coap {

// standalone Decoder works well enough, so this is largely just a netbuf-capable
// wrapper around it
template <class TNetBuf>
class NetBufDecoder : public Decoder
{
    typedef TNetBuf netbuf_t;
    typedef Decoder base_t;
    typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

protected:
    netbuf_t m_netbuf;
    // FIX: intermediate chunk until context has a value instead of a ref for its chunk
    ro_chunk_t chunk;
    Context context;

public:
    NetBufDecoder(const netbuf_t& netbuf) :
        m_netbuf(netbuf),
        chunk(netbuf.processed(), netbuf.length_processed()),
        // NOTE: Be advised that netbuf.end() differs from traditional iterator end
        // in that it is a bool indicating that we are ON the last chunk, not PAST it
        context(chunk, netbuf.end())
    {}

    netbuf_t& netbuf() { return m_netbuf; }

    bool process_iterate()
    {
        // TODO: Will know how to advance through netbuf
        return base_t::process_iterate(context);
    }

    // keep processing until we encounter state
    // keep processing for max_attempts
    // able to do this because netbuf is able to provide *all* of interested incoming buffers
    // where core decoder though it knows whether it's the last chunk, it doesn't know how
    // to retrieve the next one
    // in debug mode, this should indicate an error if we never reach said state
    bool process_until_experimental(Decoder::State s, int max_attempts = 50)
    {
        while(max_attempts-- > 0)
        {
            if(state() != s)
            {
                process_iterate();
            }
            else
                return true;
        }

        // TODO: Reveal error that we couldn't find state s
        ASSERT_ERROR(false, true, "Unable to find requested state");
        return false;
    }

    coap::Header process_header_experimental()
    {
        ASSERT_WARN(Decoder::Uninitialized, state(), "Expected to be at beginning of decoder processing");

        process_until_experimental(Decoder::HeaderDone);

        return header_decoder();
    }


    // TODO: Ensure this is a non-blocking call and return true or false denoting
    // whether we've reached a blocking point, not whether we got a nonzero tkl
    bool process_token_experimental(layer2::Token* token)
    {
        ASSERT_WARN(Decoder::HeaderDone, state(), "Expected to be at end of header processing");

        int tkl = header_decoder().token_length();

        // NOTE: Can't 100% remember if TokenDone is triggered on a 0-length token
        process_until_experimental(Decoder::TokenDone);

        if(tkl > 0)
        {
            new (token) layer2::Token(token_decoder().data(), tkl);
            return true;
        }
        else return false;
    }

    layer3::Token process_token_experimental()
    {
        ASSERT_WARN(Decoder::HeaderDone, state(), "Expected to be at end of header processing");

        int tkl = header_decoder().token_length();

        // NOTE: Can't 100% remember if TokenDone is triggered on a 0-length token
        process_until_experimental(Decoder::TokenDone);

        return layer3::Token(token_decoder().data(), tkl);
    }

    // kicks off option processing
    bool begin_option_experimental()
    {
        ASSERT_WARN(Decoder::TokenDone, state(), "Must be at end of token processing");

        // should move us into OptionsStart
        process_iterate();
        // should move us into Options or OptionsDone
        process_iterate();

        return true;
    }


    bool process_option_experimental(Option::Numbers* number, uint16_t* length)
    {
        ASSERT_WARN(Decoder::Options, state(), "Must be in options processing mode");

        process_iterate();

        ASSERT_WARN(OptionDecoder::ValueStart, option_decoder().state(), "Must be at OptionValueStart");

        *number = (Option::Numbers) option_decoder().option_delta();
        *length = option_decoder().option_length();

        return true;
    }


    ro_chunk_t get_process_option_experimental(bool* partial = NULLPTR)
    {
        ASSERT_WARN(Decoder::Options, state(), "Must be in options processing mode");

        // assert that we are at ValueStart or OptionValue
        //ASSERT_WARN(OptionDecoder::ValueStart, option_decoder().state(), "Must be at OptionValueStart");

        int value_length = option_decoder().option_length();
        const uint8_t* raw = context.chunk.data(context.pos);
        int actual_remaining_length = context.chunk.length() - context.pos;
        bool _partial = value_length > actual_remaining_length;

        if(partial != NULLPTR) *partial = _partial;

        // move forward past value portion
        base_t::process_iterate(context);

        // it's implied there's another chunk coming if value_length
        // exceeds actual_remaining_length
        if(_partial)
            return ro_chunk_t(raw, actual_remaining_length);
        else
            return ro_chunk_t(raw, value_length);
    }

    const estd::layer3::basic_string<char, false> process_option_string_experimental(Option::Numbers* number, bool* partial = NULLPTR)
    {
        uint16_t length;

        process_option_experimental(number, &length);

        ro_chunk_t option_value = get_process_option_experimental(partial);

        const estd::layer3::basic_string<char, false> s(length,
                                                        (char*) option_value.data(), length);

        return s;
    }
};

}}
