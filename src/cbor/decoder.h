#pragma once

// Revised version as of 5/17/2018

#include <stdint.h> // for uint8_t and friends
#include "../coap/uint.h"
#include <estd/string.h>
//#include "mc/memory-chunk.h"

namespace embr { namespace cbor {

struct Root
{
    // first 3 bits of header
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


    enum SimpleDataValues
    {
        Null = 22,
        True = 21,
        False = 20
    };


    // Used during non-simple-data processing
    // last 5 bits of header
    // https://tools.ietf.org/html/rfc7049#section-2
    enum AdditionalTypes
    {
        // all self contained
        bits_none = 0,

        // following represents
        //   datafield length for int in unsigned/negative integer or tag
        //   payload length for byte array, string, itemarray, map
        //   Either:
        //      16=IEEE 754 half, 32=full and 64=double precision float for float type
        //      8=32-255 simple
        // next set of bytes are:
        bits_8 = 24,
        bits_16 = 25,
        bits_32 = 26,
        bits_64 = 27,

        // not yet used
        Reserved1 = 28,
        Reserved2 = 30,

        Indefinite = 31
    };

    enum Tags
    {
        TagDateTime = 0,
        TagEpoch = 1,
        TagPosBignum = 2,
        TagNegBignum = 3
    };

    enum State
    {
        HeaderStart,
        Header,
        HeaderDone,
        AdditionalStart, // aka 'short' encoding part
        Additional,
        AdditionalDone,
        LongStart,      // TODO: Confirm this is official RFC name for this phase and if not rename it to be such
        Long,
        LongDone,
        ItemDone
    };

    static uint8_t header(Types type, AdditionalTypes additional)
    {
        return type << 5 | additional;
    }
};

namespace experimental {

class SimpleData;
class SimpleDataHelper;

}

// We've adopted terminology not really present in the RFC but indicated in the wiki
// "long field" (https://en.wikipedia.org/wiki/CBOR#Long_Field_Encoding) to indicate the difference between:
// 1. 'tiny' - all item data contained in header
// 2. 'short' - item data contained in header + data integer
// 3. 'long' - item data contained in header + length integer + data whose length is suggested by length integer
// Remember, 'long' format length is often NOT specified in bytes but rather number of data items
class Decoder : public Root
{
    friend class experimental::SimpleData;
    friend class experimental::SimpleDataHelper;

private:
#ifdef CBOR_FEATURE_64_BIT
    uint8_t buffer[8];
#else
    uint8_t buffer[4];
#endif
    struct
    {
        uint16_t m_header : 8;
        uint16_t m_pos : 3; // can count to 8, all we need for buffer
        uint16_t m_state : 4; // 16 different states should be plenty
    };

    void header(uint8_t h) { m_header = h; }

    void state(State s) { m_state = s; }


    uint8_t additional_raw() const
    {
        uint8_t result = m_header & 0b11111;

        return result;
    }

    // assuming we're using additional bits8 thru bits64, ascertain exact buffer length needed
    uint8_t additional_length() const
    {
        return 1 << (additional_raw() - 24);
    }

public:
    State state() const { return (State) m_state; }

    Decoder() : m_state(HeaderStart) {}

    Types type() const
    {
        uint8_t t = m_header >> 5;

        return (Types) t;
    }

    AdditionalTypes additional() const
    {
        return (AdditionalTypes) additional_raw();
    }

private:
    bool is_tiny_value() const
    {
        return additional_raw() < 24;
    }

    // type uses short encoding (data payload IS the 'additional' portion)
    // is_long_type() would be one where data payload FOLLOWS the additional-int portion
    // Note that a call to this implies we've already checked that !is_tiny_value()
    bool is_short_type() const
    {
        switch(type())
        {
            case UnsignedInteger:
            case NegativeInteger:
            case Tag:
            case SimpleData:
                return true;

            default:
                return false;
        }
    }

public:
    // +++
    // prefer not to use these, they sort of just clutter up the Decoder
    // even though they don't specifically violate its functionality
    bool is_simple_type() const
    {
        return type() == SimpleData;
    }

    // only callable if is_simple_type == true
    bool is_true() const
    {
        return additional_raw() == 21;
    }

    // only callable if is_simple_type == true
    bool is_false() const
    {
        return additional_raw() == 20;
    }


    // only callable if is_simple-type == true
    bool is_null() const
    {
        return additional_raw() == 22;
    }
    // ---

    // Would have used 'indefinite' but that has a different meaning in CBOR
    // indeterminate means a size *has* been specified, but exact byte count is not
    // available because even though number of items has been specified, the byte
    // size of each item is not known
    // NOTE: Consider calling this a 'container' type, since Map and ItemArray are the
    // only items which can contain other items, and thus has the more complex length behavior
    bool is_indeterminate_type() const
    {
        switch(type())
        {
            case Map:
            case ItemArray:
                return true;

            default:
                return false;
        }
    }

    template <class TInt>
    TInt integer() const
    {
        // TODO: Assert that incoming TInt has the bitness necessary
        if(is_tiny_value())
            return additional_raw();
        else if(type() == NegativeInteger)
            return -1 - coap::UInt::get<TInt>(buffer, additional_length());
        else
            return coap::UInt::get<TInt>(buffer, additional_length());
    }

    bool process_iterate(uint8_t ch);

    // put back into HeaderStart state
    void fast_forward()
    {
        state(HeaderStart);
    }

};


namespace experimental {

// NOTE: Named as such based on https://tools.ietf.org/html/rfc7049#section-1.2
// Specifically, this bears no specific relation to any mc-coap or estdlib definition of 'Stream'
class StreamDecoder
{
    Decoder item_decoder;

    //typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;
    typedef estd::const_buffer ro_chunk_t;

    friend class SimpleData;
    friend class SimpleDataHelper;

    const Decoder& decoder() const { return item_decoder; }

public:
    class Context //: public mem::ProcessedMemoryChunkBase<ro_chunk_t>
    {
        //typedef mem::ProcessedMemoryChunkBase<ro_chunk_t> base_t;

        ro_chunk_t chunk;

        // NOTE: manually managing pos instead of using ProcessedMemoryChunkBase because
        // the aforementioned is structured for write operations where unprocessed=nonconst
        // and processed=const.  We need unprocessed to also be const
        int pos;

        bool last_chunk;

    public:
        const uint8_t* unprocessed() const
        {
            //return chunk.data(pos);
            return chunk.data() + pos;
        }

        void advance(int count = 1) { pos += count; }

        //int length_unprocessed() const { return chunk.length() - pos; }
        int length_unprocessed() const { return chunk.size_bytes() - pos; }

        Context(const ro_chunk_t& chunk, bool last_chunk) :
            //base_t(chunk),
            chunk(chunk),
            pos(0),
            last_chunk(last_chunk)
        {}
    };

    bool process(Context& context);

    Root::Types type() const { return item_decoder.type(); }

    // for indeterminate types, this moves *into* the type and positions at the first contained data item
    // for non-indeterminate types, this moves *past* the type and positions at the next data item
    void fast_forward(Context& context)
    {
        ASSERT_WARN(Root::LongStart, item_decoder.state(), "Should be at start of 'long' item data");

        if(!item_decoder.is_indeterminate_type())
        {
            unsigned advance = item_decoder.integer<unsigned>();
            context.advance(advance);
        }

        item_decoder.fast_forward();
        process(context);
    }

    template <typename TInt>
    TInt integer() const { return item_decoder.integer<TInt>(); }

    // FIX: crummy state interrogation
    bool is_long_start() const
    {
        return item_decoder.state() == Root::LongStart;
    }

    estd::layer3::const_string string_experimental(Context& context)
    {
        ASSERT_WARN(Root::String, item_decoder.type(), "Expecting string type");
        ASSERT_WARN(Root::LongStart, item_decoder.state(), "Should be at start of 'long' item data");

        unsigned len = integer<unsigned>();
        estd::layer3::const_string s((const char*)context.unprocessed(), len);

        context.advance(len);
        item_decoder.fast_forward();
        process(context);
        return s;
    }
};


struct SimpleData
{
    static bool is_simple_data(const Decoder& d)
    {
        return d.type() == Decoder::SimpleData;
    }

    static bool is_simple_data(const StreamDecoder& d)
    {
        return is_simple_data(d.decoder());
    }


    static bool is_true(const Decoder& d)
    {
        return d.additional_raw() == 21;
    }


    static bool is_true(const StreamDecoder& d)
    {
        return is_true(d.decoder());
    }

    static bool is_false(const Decoder& d)
    {
        return d.additional_raw() == 20;
    }


    static bool is_false(const StreamDecoder& d)
    {
        return is_false(d.decoder());
    }
};


// NOTE: Already don't like using this, was thinking it might be helpful to isolate decoding
// stuff outside Decoder itself for SimpleData specific scenarios
class SimpleDataHelper
{
    uint8_t additional;

public:
    SimpleDataHelper(const StreamDecoder& d)
    {
        ASSERT_WARN(d.decoder().type(), Decoder::SimpleData, "Invalid type encountered");

        additional = d.decoder().additional_raw();
    }

    bool is_true() const { return additional == 21; }
    bool is_false() const { return additional == 20; }
    bool is_null() const { return additional == 22; }
};





}

}}
