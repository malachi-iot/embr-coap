#pragma once

// Revised version as of 5/17/2018

#include <stdint.h> // for uint8_t and friends
#include <coap-uint.h>

namespace moducom { namespace cbor {

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


    // Used during non-simple-data processing
    // last 5 bits of header
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

    enum State
    {
        HeaderStart,
        Header,
        HeaderDone,
        AdditionalStart, // aka 'short' encoding part
        Additional,
        AdditionalDone,
        LongStart,
        Long,
        LongDone,
        ItemDone
    };

    static uint8_t header(Types type, AdditionalTypes additional)
    {
        return type << 5 | additional;
    }
};

class Decoder : public Root
{
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

    // assuming we're using bits8 thru bits64, ascertain exact buffer length needed
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
    // Would have used 'indefinite' but that has a different meaning in CBOR
    // indeterminate means a size *has* been specified, but exact byte count is not
    // available because even though number of items has been specified, the byte
    // size of each item is not known
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
    TInt integer()
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

class OverallDecoder
{
    Decoder item_decoder;

    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;

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
            return chunk.data(pos);
        }

        void advance(int count = 1) { pos += count; }

        int length_unprocessed() const { return chunk.length() - pos; }

        Context(const ro_chunk_t& chunk, bool last_chunk) :
            //base_t(chunk),
            chunk(chunk),
            pos(0),
            last_chunk(last_chunk)
        {}
    };

    bool process(Context& context);

    Root::Types type() const { return item_decoder.type(); }
};

}

}}
