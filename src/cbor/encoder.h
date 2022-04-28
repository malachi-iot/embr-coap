#pragma once

#include "../cbor.h"
#include "decoder.h"
#include "mc/encoder-base.h"

namespace embr { namespace cbor {

class EncoderBaseBase :
        public embr::EncoderBase<>,
        public Root
{
    typedef embr::EncoderBase<> base_t;
protected:
    typedef CBOR::Types types_t;
    typedef CBOR::Decoder::AdditionalIntegerInformation add_int_info_t;
    typedef typename base_t::size_type size_type;

    // for scenarios where we know we're dealing with a 'tiny' field encoding scenario
    void tiny_field(uint8_t* d, types_t type, uint8_t value)
    {
        *d = type << 5 | value;
        written(1);
    }

    // for scenarios where we know we're dealing with a 'short' field encoding scenario

public:
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


    // encodes into 'd' buffer 'tiny' or 'short' type data
    // (see https://en.wikipedia.org/wiki/CBOR#CBOR_data_item_field_encoding)
    // use this also for the header + length portion of 'long' type data
    //
    // returns number of bytes occupied - remember, this excludes "contents"
    // such as array items, string contents, etc.
    //
    // TODO: Make a specialization of this for SimpleData type which is *always*
    // in tiny mode
    template <typename TInt>
    bool major_type_and_integer(uint8_t* d, size_type len, types_t type, TInt value)
    {
        // TODO: issue a compiler error if TInt is not unsigned at this
        // point

        if(value <= 24)
        {
            tiny_field(d, type, value);
            return true;
        }
        else
        {
            uint8_t bytes_used = coap::UInt::assess_bytes_used(value);
            switch(bytes_used)
            {
                case 1:
                case 2:
                case 4:
                case 8:
                    // no need to fiddle, 8, 16, 32 bit word values exist
                    break;

                case 3:
                    bytes_used = 4; // pad to full 32 bit, 24 bit word values we don't use
                    break;

                case 5:
                case 6:
                case 7:
                    bytes_used = 8;

                default:
                    // TODO: make this an error
                    break;
            }

            *d++ = type << 5 | additional_integer_information_from_wordsize(bytes_used);
            coap::UInt::set(value, d, bytes_used);
            written(bytes_used + 1);
            return true;
        }
    }
};



template <class TBuffer>
class EncoderBase : public EncoderBaseBase
{
protected:
    TBuffer buffer;
    typedef EncoderBaseBase base_t;

public:
    uint8_t* data() { return buffer; }

protected:
    typedef CBOR::Types types_t;
    typedef CBOR::Decoder::AdditionalIntegerInformation add_int_info_t;


public:
    template <typename TInt>
    uint8_t major_type_and_integer(types_t type, TInt value)
    {
        bool result = base_t::major_type_and_integer(data(), -1, type, value);

        return base_t::m_written;
    }

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
