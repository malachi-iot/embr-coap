//
// Created by malachi on 1/3/18.
//

#ifndef MC_COAP_TEST_COAP_OBSERVABLE_H
#define MC_COAP_TEST_COAP_OBSERVABLE_H

#include "coap.h"
#include "coap-token.h"

namespace moducom { namespace  coap {

// Not seeing anything specific in RFC7641 mandating or denying the use of CON or NON
// messaging for observables, thereby implying we don't need to track what flavor
// we are in this context either
struct ObservableContext
{
    // increases with each observe notification.  Technically only needs to be 24 bits
    // so if we do need flags, we can make this into a bit field.  Also, as suggested
    // in RFC7641, we might do without a sequence# altogether and instead use a mangled
    // version of system timestamp.  In that case, holding on to sequence# here might
    // be a waste.  What does not seem to be specified at all is how to
    // handle rollover/overflow conditions.
    uint32_t sequence;

    layer1::Token token;
};

}}

#endif //MC_COAP_TEST_COAP_OBSERVABLE_H
