#pragma once

#include "../cbor.h"

namespace moducom { namespace cbor {

template <class TBuffer>
class EncoderBase
{
protected:
    TBuffer buffer;

public:
    uint8_t* data() { return buffer; }

protected:
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

    typedef CBOR::Types types_t;
    typedef CBOR::Decoder::AdditionalIntegerInformation add_int_info_t;

    // returns number of bytes occupied - remember, this excludes "contents"
    // such as array items, string contents, etc.
    template <typename TInt>
    uint8_t major_type_and_integer(types_t type, TInt value)
    {
        uint8_t* d = data();

        // TODO: issue a compiler error if TInt is not unsigned at this
        // point

        if(value <= 24)
        {
            *d = type << 5 | value;
            return 1;
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
            return bytes_used + 1;
        }
    }

public:

    template <typename TInt>
    uint8_t integer(TInt value)
    {
        if(value >= 0)
            return major_type_and_integer(CBOR::UnsignedInteger, value);
        else
            return major_type_and_integer(CBOR::NegativeInteger, -value);
    }


    // NOTE: this only encodes string preamble, literal string contents
    // itself are outside the scope of this encoder
    uint8_t string(unsigned length)
    {
        return major_type_and_integer(CBOR::String, length);
    }


    // NOTE: this only encodes array preamble, literal string contents
    // itself are outside the scope of this encoder
    uint8_t array(unsigned length)
    {
        return major_type_and_integer(CBOR::ByteArray, length);
    }
};


namespace layer1 {

class Encoder : public cbor::EncoderBase<uint8_t[5]>
{

};

}


namespace layer3 {

class Encoder : public cbor::EncoderBase<uint8_t*>
{
public:
    Encoder(uint8_t* buffer) { this->buffer = buffer; }
};

}

}}