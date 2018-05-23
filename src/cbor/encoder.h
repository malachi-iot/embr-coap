#pragma once

#include "../cbor.h"
#include "mc/encoder-base.h"

namespace moducom { namespace cbor {

class EncoderBaseBase : public moducom::EncoderBase<>
{
    typedef moducom::EncoderBase<> base_t;
    typedef CBOR::Types types_t;
    typedef CBOR::Decoder::AdditionalIntegerInformation add_int_info_t;
    typedef typename base_t::size_type size_type;

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


    // returns number of bytes occupied - remember, this excludes "contents"
    // such as array items, string contents, etc.
    template <typename TInt>
    bool major_type_and_integer(uint8_t* d, size_type len, types_t type, TInt value)
    {
        // TODO: issue a compiler error if TInt is not unsigned at this
        // point

        if(value <= 24)
        {
            *d = type << 5 | value;
            written(1);
            return true;
        }
        else
        {
            d++;
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
            coap::UInt::set(value, d, bytes_used);

            d--;
            *d = type << 5 | additional_integer_information_from_wordsize(bytes_used);
            written(bytes_used + 1);
            return true;
        }
    }
};

template <class TNetBuf>
class NetBufEncoder :
        public moducom::io::experimental::NetBufWriter<TNetBuf>,
        EncoderBaseBase
{
    typedef EncoderBaseBase base_t;
    typedef moducom::io::experimental::NetBufWriter<TNetBuf> nbw_t;
    typedef typename nbw_t::netbuf_t netbuf_t;

public:
    template <class TChar, class TTraits, class TAllocator>
    bool string(const estd::basic_string<TChar, TTraits, TAllocator>& insert_from)
    {
        netbuf_t& netbuf = nbw_t::netbuf();

        bool success = base_t::major_type_and_integer(
                netbuf.unprocessed(),
                netbuf.length_unprocessed(),
                CBOR::String, insert_from.size());

        if(!success) return false;

        nbw_t::advance(m_written);

        // TODO: check for success here
        int copied = insert_from.copy((char*)netbuf.unprocessed(), netbuf.length_unprocessed());

        base_t::m_written += copied;
        nbw_t::advance(copied);

        return copied == insert_from.size();
    }

    template <typename TInt>
    bool integer(TInt value)
    {
        netbuf_t& netbuf = nbw_t::netbuf();
        bool result;

        if(value >= 0)
            result = major_type_and_integer(netbuf.unprocessed(),
                                          netbuf.length_unprocessed(),
                                          CBOR::UnsignedInteger, value);
        else
            result = major_type_and_integer(netbuf.unprocessed(),
                                          netbuf.length_unprocessed(),
                                          CBOR::NegativeInteger,
                                          -value - 1);

        nbw_t::advance(base_t::m_written);
        return result;
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
