#pragma once

// Revised version as of 5/17/2018

#include <stdint.h> // for uint8_t and friends

namespace moducom { namespace cbor {

class Decoder
{
public:
    bool process_iterate(uint8_t ch);
};

}}
