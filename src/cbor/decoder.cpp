#include "decoder.h"


namespace embr { namespace cbor {

bool Decoder::process_iterate(uint8_t ch)
{
    switch(state())
    {
        case HeaderStart:
            state(Header);
            break;

        case Header:
            header(ch);
            state(HeaderDone);
            return true;

        case HeaderDone:
            if(is_tiny_value())
            {
                if(is_short_type())
                    state(ItemDone);
                else
                    state(LongStart);
            }
            else
                state(AdditionalStart);
            break;

        case AdditionalStart:
            m_pos = 0;
            state(Additional);
            break;

        case Additional:
            buffer[m_pos++] = ch;
            if(m_pos == additional_length())
                state(AdditionalDone);

            return true;

        case AdditionalDone:
            if(is_short_type())
                state(ItemDone);
            else
                state(LongStart);
            break;

        case LongStart:
            break;

        case Long:
            break;

        case LongDone:
            break;

        case ItemDone:
            state(HeaderStart);
            break;
    }

    return false;
}

namespace experimental {

bool StreamDecoder::process(Context& context)
{
    while(context.length_unprocessed() > 0 ||
            (context.length_unprocessed() == 0 && item_decoder.state() != Decoder::ItemDone))
    {
        // NOTE: processed memory chunk seems almost-suited but it's reversed,
        // in our case 'unprocessed' memory is the one that decoder hasn't decoded yet,
        // but with processed memory that is non-const
        //if(item_decoder.process_iterate(*context.unprocessed()))
        //  context.advance(1);
        if(item_decoder.process_iterate(*context.unprocessed()))
            context.advance();

        switch(item_decoder.state())
        {
            case Decoder::Header:
                break;

            case Decoder::HeaderDone:
                break;

            case Decoder::LongStart:
                return false;

            case Decoder::ItemDone:
                return false;

            default:
                break;
        }
    }

    return true;
}

}


}}
