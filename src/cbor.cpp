#include "cbor.h"

namespace moducom {

bool CBOR::DecoderStateMachine::process_iterate(uint8_t value)
{
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
                            // FIX: Not yet supported, need to make sure I get endian behavior right
                            break;
                    }
                    break;

                case ItemArray:
                    state(ItemArrayState);
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
                            // FIX: Not yet supported, need to make sure I get endian behavior right
                            break;
                    }
                    // can't get byte count because items in array are variable
                    break;

                default:
                    state(MajorType);
            }
            return false;
        }

        case ByteArrayState:
        {
            if(--count == 0)
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
            if(--count == 0)
                state(ItemArrayDone);

            return true;
        }
    }
    return false;
}


}
