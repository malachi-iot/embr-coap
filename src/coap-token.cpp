//
// Created by malachi on 12/29/17.
//

#include "coap/token.h"
#include "coap/uint.h"

namespace moducom { namespace coap {

void SimpleTokenGenerator::generate(layer2::Token* token)
{
    //uint8_t* data = (uint8_t*)token->data();
    // FIX: Optimize, set up a prepar operation elsewhere
    layer2::Token& data = *token;

    int bytes_used = UInt::assess_bytes_used(current);
    data.resize(bytes_used);

    UInt::set(current++, data);
}

}}
