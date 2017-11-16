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
    CoAPGenerator generator;
    pipeline::IPipeline& output;

public:
    GeneratorResponder(pipeline::IPipeline& output) :
        output(output),
        generator(output)
    {}

    bool process_iterate();
};

}}


