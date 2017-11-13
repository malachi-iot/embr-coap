//
// Created by malachi on 11/13/17.
//

#ifndef MC_COAP_TEST_COAP_GENERATOR_H
#define MC_COAP_TEST_COAP_GENERATOR_H

#include "mc/pipeline.h"

namespace moducom { namespace coap {

class CoAPGenerator
{
    pipeline::IPipeline& output;
    //experimental::layer2::OptionGenerator::StateMachine option_state;

public:
    CoAPGenerator(pipeline::IPipeline& output) : output(output) {}

    bool output_option_iterate(experimental::layer2::OptionBase& option);
};

}}

#endif //MC_COAP_TEST_COAP_GENERATOR_H
