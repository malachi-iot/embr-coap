#include "cbor.h"

namespace moducom {

void CBOR::DecoderStateMachine::assign_additional_integer_information()
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


bool CBOR::DecoderStateMachine::process_iterate(uint8_t value)
{
    bool encountered_break;

    DecoderStateMachine* nested = this->lock_nested();

    if(nested)
    {
        bool processed = nested->process_iterate(value);
        if(nested->state() == Pop)
        {
            encountered_break = nested->encountered_break();

            // then we are actually done
            // de-allocated "nested" memory
            free_nested();
        }
        else
            unlock_nested();

        return processed;
    }
    // don't need to unlock or free nested if it was never allocated, just proceed

    switch(state())
    {
        case MajorType:
        {
            buffer[0] = value;
            pos = 1;

            return true;
        }

        case MajorTypeDone:
            if(has_additional_integer_information())
                state(AdditionalInteger);

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
                    {
                        state(AdditionalIntegerDone);
                        return false;
                    }
                case bits_16:
                    if(pos == 3)
                    {
                        state(AdditionalIntegerDone);
                        return false;
                    }
                case bits_32:
                    if(pos == 5)
                    {
                        state(AdditionalIntegerDone);
                        return false;
                    }
                case bits_64:
                    if(pos == 9)
                    {
                        state(AdditionalIntegerDone);
                        return false;
                    }
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
                    assign_additional_integer_information();
                    break;

                default:
                    state(MajorType);
            }
            return false;
        }

        case ByteArrayState:
        {
            // something feels wrong about doing this here
            if(is_indefinite())
            {
                if(encountered_break)
                    state(ByteArrayDone);
            }
            else if(--count == 0)
                state(ByteArrayDone);

            return true;
        }

        case ByteArrayDone:
            state(MajorType);
            return false;

        case ItemArrayState:
        {
            // will probably need nested substates because we can have arrays of arrays and
            // itemarrays of itemarrays.  that is gonna be tricky in a fixed-size state machine
            if(is_indefinite())
            {

            }
            else if(--count == 0)
                state(ItemArrayDone);

            return true;
        }

        case ItemArrayDone:
        {
            state(MajorType);
            return false;
        }

        case MapState:
        {
            return true;
        }

        case MapDone:
        {
            state(MajorType);
            return false;
        }

        case Pop: return false;
    }
    return false;
}


}
