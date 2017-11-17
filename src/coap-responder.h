//
// Created by malachi on 11/15/17.
//

#pragma once

#include "coap_transmission.h"
#include "coap-generator.h"
#include "coap_daemon.h"

namespace moducom { namespace coap {


class GeneratorResponder : public CoAP::IResponder
{
protected:
    struct header_state_t
    {
        uint8_t bytes[4];
    };

    struct token_state_t
    {
        // FIX: instead reuse someone else's allocated token if we can
        // *and* if it's practical
        uint8_t token[8];
        uint8_t length;
    };

    union
    {
        header_state_t header;
        token_state_t token;
    } extra;

    CoAPGenerator generator;

    typedef CoAP::Parser::State state_t;
    typedef CoAPGenerator::option_t option_t;
    typedef CoAPGenerator::option_number_t option_number_t;

#ifdef __CPP11__
    typedef state_t _state_t;
#else
    typedef CoAP::Parser _state_t;
#endif

    state_t _state;

    state_t state() const { return _state; }
    void state(state_t state) { _state = state; }

public:
    GeneratorResponder(pipeline::IPipeline& output) :
        _state(_state_t::Header),
        generator(output)
    {}

    // handle header/token portion
    bool process_iterate();

    virtual void OnHeader(const CoAP::Header header) OVERRIDE;
    virtual void OnToken(const uint8_t message[], size_t length) OVERRIDE;
};

}}


