//
// Created by malachi on 11/15/17.
//

#include "coap-responder.h"

namespace moducom { namespace coap {

bool GeneratorResponder::process_header_and_token_iterate()
{
    switch(state())
    {
        case _state_t::Header:
        {
            if(generator.output_header_iterate())
                state(_state_t::HeaderDone);
            break;
        }

        case _state_t::HeaderDone:
        {
            if(generator.get_header()->token_length() > 0)
            {
                generator.output_token_begin();
                state(_state_t::Token);
            }
            else
            {
                state(_state_t::OptionsStart);
            }

            break;
        }

        case _state_t::Token:
        {
            pipeline::MemoryChunk chunk(extra.token.token, extra.token.length);
            if(generator.output_token_iterate(chunk))
                state(_state_t::TokenDone);
            break;
        }

        case _state_t::TokenDone:
            state(_state_t::OptionsStart);
            break;

        case _state_t::OptionsStart:
            return true;

            // FIX: Put assert here, shouldn't get here
        default:
            return true;
    }

    return false;
}


// NOTE: This is outdated, to be replaced by isolated OptionEncoderHelper
bool GeneratorResponder::option_iterate(const option_t &option)
{
    bool retVal = false;

    if(state() == _state_t::OptionsStart)
    {
        generator.output_option_begin(option);
        state(_state_t::Options);
    }
    else if(state() == _state_t::Options)
    {
        CoAP::ParserDeprecated::SubState sub_state = generator.get_option_state().state();

        if(sub_state == CoAP::ParserDeprecated::OptionValueDone)
        {
            generator.output_option_next(option);
        }

        generator.output_option_iterate();

        sub_state = generator.get_option_state().state();

        if(sub_state == CoAP::ParserDeprecated::OptionValueDone)
        {
            // FIX: Not perfect, since this very well may be the last option
            state(_state_t::OptionsStart);
            retVal = true;
        }
    }

    return retVal;
}


void GeneratorResponder::OnHeader(const CoAP::Header header)
{
    memcpy(extra.header.bytes, header.bytes, 4);
    // beware that because I am using unions for extra.** that OnToken
    // cannot happen until after process_header_and_token_iterate has serviced this header, so in
    // other words currently The OnX functions must be synchronized with
    // process_header_and_token_iterate
    //
    // that being said, we don't actually use the extra.header portion
    // (header is cached in the generator) and it's looking like it might
    // be convenient to cache the token in the generator too (probably won't
    // take much extra information)
    //
    // Be aware also that ParseToIResponderDeprecated::process sends everything off
    // at once, which does not give us a chance to interleave.  Luckily because
    // of aforementioned trickery caching header bytes in the generator,
    // that doesn't hurt us
    generator.output_header_begin(header);
}


void GeneratorResponder::OnToken(const uint8_t* message, size_t length)
{
    // if we get a token, remember it for later with our response
    memcpy(extra.token.token, message, length);
    extra.token.length = length;
    generator.output_token_begin();
}


namespace experimental
{

void NonBlockingSender::send()
{
    while(!send_option());
    while(!send_payload());
}

}

}}