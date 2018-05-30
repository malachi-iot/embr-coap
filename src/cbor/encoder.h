#pragma once

#include "../cbor.h"
#include "decoder.h"
#include "mc/encoder-base.h"

namespace moducom { namespace cbor {

class EncoderBaseBase :
        public moducom::EncoderBase<>,
        public Root
{
    typedef moducom::EncoderBase<> base_t;
protected:
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
            *d = type << 5 | value;
            written(1);
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

template <class TNetBuf>
class NetBufEncoder :
        public moducom::io::experimental::NetBufWriter<TNetBuf>,
        EncoderBaseBase
{
protected:
    typedef EncoderBaseBase base_t;
    typedef moducom::io::experimental::NetBufWriter<TNetBuf> nbw_t;
    typedef typename nbw_t::netbuf_t netbuf_t;
    typedef typename base_t::types_t types_t;

protected:
    // encodes 'tiny' or 'short' type data into our netbuf
    template <typename TInt>
    bool major_type_and_integer(types_t type, TInt value)
    {
        netbuf_t& netbuf = nbw_t::netbuf();

        bool success = base_t::major_type_and_integer(
                netbuf.unprocessed(),
                netbuf.length_unprocessed(),
                type, value);

        nbw_t::advance(m_written);

        return success;
    }

public:
    bool null()
    {
        return major_type_and_integer(CBOR::SimpleData, (uint8_t) base_t::Null);
    }

    bool boolean(bool value)
    {
        uint8_t raw_value = value ? base_t::True : base_t::False;

        return major_type_and_integer(CBOR::SimpleData, raw_value);
    }

    bool string_header(int len)
    {
        return major_type_and_integer(CBOR::String, len);
    }

    bool string(const char* s)
    {
        int len = strlen(s);

        if(!string_header(len)) return false;

        // TODO: check for success here
        int copied = nbw_t::write(s, len);

        base_t::m_written += copied;

        return copied == len;
    }

    template <class TChar, class TTraits, class TAllocator>
    bool string(const estd::basic_string<TChar, TTraits, TAllocator>& insert_from)
    {
        netbuf_t& netbuf = nbw_t::netbuf();
        int len = insert_from.size();

        if(!string_header(len)) return false;

        // TODO: check for success here
        int copied = insert_from.copy((char*)netbuf.unprocessed(), netbuf.length_unprocessed());

        base_t::m_written += copied;
        nbw_t::advance(copied);

        return copied == len;
    }

    template <typename TInt>
    bool integer(TInt value)
    {
        if(value >= 0)
            return major_type_and_integer(CBOR::UnsignedInteger, value);
        else
            return major_type_and_integer(CBOR::NegativeInteger,
                                          -value - 1);
    }

    template <typename TInt>
    bool map(TInt count)
    {
        return major_type_and_integer(CBOR::Map, count);
    }

    // NOTE: this only encodes array preamble, literal string contents
    // itself are outside the scope of this encoder
    bool array_header(unsigned length)
    {
        return major_type_and_integer(CBOR::ByteArray, length);
    }

    // NOTE: this only encodes array preamble, literal string contents
    // itself are outside the scope of this encoder
    bool items_header(unsigned length)
    {
        return major_type_and_integer(CBOR::ItemArray, length);
    }

#ifdef FEATURE_CPP_INITIALIZER_LIST
    template <class T>
    bool array(std::initializer_list<T> l)
    {
        array_header(l.size());
        uint8_t* d = nbw_t::netbuf().unprocessed();
        for(auto v : l)
        {
            *d++ = v;
        }
        written(l.size());
        nbw_t::advance(l.size());
        return true;
    }
#endif

    template <int N>
    bool array(uint8_t (&a)[N])
    {
        array(N);
        int w = nbw_t::write(a);
        written(w);
        return true;
    }

#ifdef FEATURE_CPP_INITIALIZER_LIST
    template <class T>
    bool items(std::initializer_list<T> l)
    {
        items_header(l.size());
        uint8_t* d = nbw_t::netbuf().unprocessed();
        for(auto v : l)
        {
            *d++ = v;
        }
        written(l.size());
        nbw_t::advance(l.size());
        return true;
    }
#endif

    template <int N>
    bool items(uint8_t (&a)[N])
    {
        items(N);
        int w = nbw_t::write(a);
        written(w);
        return true;
    }};


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
