#pragma once

#include "../cbor.h"

namespace moducom { namespace cbor {

template <class TBuffer>
class EncoderBase
{
    TBuffer buffer;

    uint8_t* data() { return buffer; }

    static uint8_t additional_integer_information_from_wordsize(uint8_t wordsize)
    {
        switch(wordsize)
        {
            case 1: return CBOR::Decoder::bits_8;
            case 2: return CBOR::Decoder::bits_16;
            case 4: return CBOR::Decoder::bits_32;
            case 8: return CBOR::Decoder::bits_64;

            default:
                // TODO: Generate an error here
                return 0;
        }
    }

protected:
    typedef CBOR::Types types_t;
    typedef CBOR::Decoder::AdditionalIntegerInformation add_int_info_t;

    template <typename TInt>
    void major_type_and_integer(types_t type, TInt value)
    {
        uint8_t* d = data();

        // TODO: issue a compiler error if TInt is not unsigned at this
        // point

        if(value <= 24)
        {
            *d = type << 5 | value;
        }
        else
        {
            d++;
            uint8_t bytes_used = coap::UInt::assess_bytes_used(value);
            switch(bytes_used)
            {
                case 1:
                case 2:
                    // no need to fiddle, 8 and 16 bit word values exist
                    break;
                case 3:
                    bytes_used = 4; // pad to full 32 bit, 24 bit word values we don't use
                    break;

                default:
                    // TODO: make this an error
                    break;
            }
            coap::UInt::set(value, d, bytes_used);

            d--;
            *d = type << 5 | additional_integer_information_from_wordsize(bytes_used);
        }
    }

public:

    template <typename TInt>
    void integer(TInt value)
    {
        if(value >= 0)
            major_type_and_integer(CBOR::UnsignedInteger, value);
        else
            major_type_and_integer(CBOR::NegativeInteger, -value);
    }


    void string(int length)
    {

    }
};

}}