#pragma once

#include "../coap-decoder.h"
#include "../coap-token.h"

namespace moducom { namespace coap {

// standalone Decoder works well enough, so this is largely just a netbuf-capable
// wrapper around it
template <class TNetBuf>
class NetBufDecoder : public Decoder
{
    typedef TNetBuf netbuf_t;
    typedef Decoder base_t;

protected:
    netbuf_t m_netbuf;
    // FIX: intermediate chunk until context has a value instead of a ref for its chunk
    moducom::pipeline::MemoryChunk::readonly_t chunk;
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
    bool process_option_experimental()
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
        ASSERT_WARN(OptionDecoder::ValueStart, option_decoder().state(), "Must be at OptionValueStart");

        *number = (Option::Numbers) option_decoder().option_delta();
        *length = option_decoder().option_length();

        return true;
    }
};

}}
