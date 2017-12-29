//
// Created by malachi on 12/29/17.
//

#ifndef MC_COAP_TEST_COAP_TOKEN_H
#define MC_COAP_TEST_COAP_TOKEN_H

#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "mc/memory-chunk.h"

namespace moducom { namespace coap { namespace layer2 {

// https://tools.ietf.org/html/rfc7252#section-5.3.1
// semi-duplicate of one in coap_transmission
class Token : public moducom::pipeline::layer2::MemoryChunk<8, uint8_t>
{
};


}

class SimpleTokenGenerator
{
    uint32_t current;

public:
    SimpleTokenGenerator() : current(0) {}

    void generate(coap::layer2::Token* token);

#ifdef DEBUG
    void set(uint32_t current) { this->current = current; }
#endif
};



}}


#endif //MC_COAP_TEST_COAP_TOKEN_H
