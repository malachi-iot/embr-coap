#pragma once

// Revised version as of 5/17/2018

#include <stdint.h> // for uint8_t and friends

namespace moducom { namespace cbor {

struct Root
{
    enum Types
    {
        UnsignedInteger = 0,
        PositiveInteger = 0,
        NegativeInteger = 1,
        ByteArray = 2,
        String = 3,
        ItemArray = 4,
        Map = 5,
        Tag = 6,
        Float = 7,
        SimpleData = 7
    };


    // Used during non-simple-data processing
    enum AdditionalIntegerInformation
    {
        // all self contained
        bits_none = 0,

        bits_8 = 24,
        bits_16 = 25,
        bits_32 = 26,
        bits_64 = 27,
        Reserved1 = 28,
        Reserved2 = 30,
        Indefinite = 31
    };
};

class Decoder : public Root
{
public:
    bool process_iterate(uint8_t ch);
};

}}
