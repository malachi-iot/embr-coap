#include "cbor.h"

namespace moducom {

void CBOR::Decoder::assign_additional_integer_information()
{
    switch(additional_integer_information())
    {
        case bits_8:
            count = get_value_8();
            break;

        case bits_16:
            count = get_value_16();
            break;

        case bits_32:
        case bits_64:
#ifdef CBOR_FEATURE_32_BIT
#error "32-bit and higher values not yet supported"
#endif
            // FIX: Not yet supported, need to make sure I get endian behavior right
            break;
    }
}

bool CBOR::Decoder::process_iterate_nested(uint8_t value, bool* encountered_break)
{
    // nested decoder lasts the duration of the array/map which uses it
    // avoid alloc/free-ing per item
    Decoder* nested = this->lock_nested();

    ASSERT_ERROR(true, nested != NULLPTR, "Expected nested decoder but none present");

    bool processed = nested->process_iterate(value);
    if(nested->state() == Pop)
    {
        *encountered_break = nested->encountered_break();

        // then we are actually done
        // de-allocated "nested" memory
        free_nested();
    }
    else
        unlock_nested();

    return processed;
}


bool CBOR::Decoder::process_iterate(uint8_t value)
{
    switch(state())
    {
        case MajorType:
        {
            buffer[0] = value;
            pos = 1;

            state(MajorTypeDone);

            return true;
        }

        case MajorTypeDone:
            if(has_additional_integer_information())
                state(AdditionalInteger);
            else
                state(Done);

            return false;

        case AdditionalInteger:
        {
            // if we reach compare_to_pos, we are done
            //const uint8_t compare_to_pos = (additional_integer_information() - 23);
            // then need to do (2 exp compare_to_pos)

            buffer[pos++] = value;

            /*
            if(pos == compare_to_pos)
            {
                state(AdditionalIntegerDone);
                return false;
            } */

            switch(additional_integer_information())
            {
                case bits_8:
                    if(pos == 2)
                        state(AdditionalIntegerDone);
                    break;

                case bits_16:
                    if(pos == 3)
                        state(AdditionalIntegerDone);
                    break;

                case bits_32:
                    if(pos == 5)
                        state(AdditionalIntegerDone);
                    break;

                case bits_64:
                    if(pos == 9)
                        state(AdditionalIntegerDone);
                    break;
            }
            return true;
        }

        case AdditionalIntegerDone:
        {
            switch(type())
            {
                case ByteArray:
                case String:
                    state(ByteArrayState);
                    if(is_indefinite())
                        // indefinite byte arrays are treated as a batch of regular
                        // arrays, and therefore are nested
                        alloc_nested();
                    else
                        assign_additional_integer_information();
                    break;

                case ItemArray:
                    state(ItemArrayState);
                    assign_additional_integer_information();
                    // each item in this array is processed as an independent CBOR decoder state machine
                    alloc_nested();
                    // can't get byte count because items in array are variable
                    break;

                case Map:
                    state(MapState);
                    alloc_nested();
                    assign_additional_integer_information();
                    break;

                case Tag:
                    state(MajorType);
                    break;

                default:
                    state(Done);
                    break;
            }
            return false;
        }

        case ByteArrayState:
        {
            if(is_indefinite())
            {
                // indefinite byte arrays are a sequence of CBOR regular arrays ended by the 0xFF break
                // and therefore are nested
                Decoder* nested = lock_nested();

                // if the last processing step yielded DONE
                // and we now have a STOP/BREAK value
                if(value == 0xFF && nested->state() == Done)
                {
                    unlock_nested();
                    free_nested();

                    state(ByteArrayDone);
                    return true;
                }

                bool processed = nested->process_iterate(value);

                unlock_nested();

                return processed;
            }
            else if(--count == 0)
                state(ByteArrayDone);

            return true;
        }

        case ByteArrayDone:
            state(Done);
            return false;

        case ItemArrayState:
        {
            Decoder* nested = lock_nested();

            bool processed = nested->process_iterate(value);

            if(nested->state() == Done)
            {
                if(is_indefinite())
                {
                    // it won't have processed it yet, it needs to move
                    // back to MajorType first for that
                    //if(nested->encountered_break())
                    if(value == 0xFF)
                        state(ItemArrayDone);
                }
                else if(--count == 0)
                    state(ItemArrayDone);
            }

            unlock_nested();

            return processed;
        }

        case ItemArrayDone:
        {
            free_nested();
            state(Done);
            return false;
        }

        case MapState:
        {
            return true;
        }

        case MapDone:
        {
            state(Done);
            return false;
        }

        case Done:
            state(MajorType);
            return false;

        case Pop: return false;
    }
    return false;
}


void CBOR::DecodeToObserver::decode(const uint8_t *cbor, size_t len)
{
#ifdef __CPP11__
#else
    typedef Decoder state_t;
    typedef CBOR type_t;
#endif

    Decoder decoder;

    for(int i = 0; i < len; i++)
    {
        while(!decoder.process_iterate(cbor[i]))
        {
            switch(decoder.state())
            {
                case state_t::Done:
                    switch(decoder.type())
                    {
                        case type_t::UnsignedInteger:
                        case type_t::NegativeInteger:
                        case type_t::Float:
                            observer->OnType(decoder);
                            break;
                    }
                    break;
            }
        }
    }
}

}
