//
// Created by malachi on 11/15/17.
//

#include "coap-responder.h"

namespace moducom { namespace coap {

bool GeneratorResponder::process_iterate()
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


bool GeneratorResponder::option_iterate(const option_t &option)
{
    if(state() == _state_t::OptionsStart)
    {
        generator.output_option_begin(option);
        state(_state_t::Options);
    }
    // FIX: Rest of option_iterate needs to be build out
}


void GeneratorResponder::OnHeader(const CoAP::Header header)
{
    memcpy(extra.header.bytes, header.bytes, 4);
    // beware that because I am using unions for extra.** that OnToken
    // cannot happen until after process_iterate has serviced this header, so in
    // other words currently The OnX functions must be synchronized with
    // process_iterate
    //
    // that being said, we don't actually use the extra.header portion
    // (header is cached in the generator) and it's looking like it might
    // be convenient to cache the token in the generator too (probably won't
    // take much extra information)
    //
    // Be aware also that ParseToIResponder::process sends everything off
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