//
// Created by malachi on 11/15/17.
//

#pragma once

#include "coap_transmission.h"
#include "coap-generator.h"

namespace moducom { namespace coap {

class GeneratorResponder : public CoAP::IResponder
{
    CoAPGenerator generator;

public:
};

}}


