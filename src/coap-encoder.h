//
// Created by Malachi Burke on 11/24/17.
// Encoders spit out encoded bytes based on a more meaningful set of inputs
//

#ifndef MC_COAP_TEST_COAP_ENCODER_H
#define MC_COAP_TEST_COAP_ENCODER_H

#include "coap_transmission.h"

namespace moducom { namespace coap {

//typedef experimental::layer2::OptionGenerator::StateMachine OptionEncoder;
class OptionEncoder : public experimental::layer2::OptionGenerator::StateMachine
{
public:
    bool process_iterate(pipeline::IBufferedPipelineWriter& writer);
};

}}

#endif //MC_COAP_TEST_COAP_ENCODER_H
