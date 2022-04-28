#pragma once

#include <cbor/encoder.h>

namespace embr { namespace cbor {

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
    bool tiny_field(types_t type, uint8_t value)
    {
        netbuf_t& netbuf = nbw_t::netbuf();

        base_t::tiny_field(
                netbuf.unprocessed(),
                type, value);

        nbw_t::advance(m_written);

        return true;
    }

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
        return tiny_field(CBOR::SimpleData, base_t::Null);
    }

    bool boolean(bool value)
    {
        return tiny_field(CBOR::SimpleData, value ? base_t::True : base_t::False);
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

    template <class TChar, class TStringTraits, class TAllocator>
    bool string(const estd::basic_string<TChar, typename TStringTraits::char_traits, TAllocator, TStringTraits>& insert_from)
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
    }
};

}}