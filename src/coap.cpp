#include "coap.h"
#include "mc/memory.h"

namespace moducom {
namespace coap {

const char* get_description(Option::Numbers number)
{
    switch(number)
    {
        case Option::Zeroed:        return "Zeroed";
        case Option::ETag:          return "ETag";
        case Option::Observe:       return "Observe";
        case Option::UriPath:       return "Uri-Path";
        case Option::UriQuery:      return "Uri-Query";
        case Option::Block1:        return "Block1";
        case Option::Block2:        return "Block2";
        default:                    return NULLPTR;
    }
}


const char* get_description(internal::Root::State state)
{
    using namespace internal;

    switch(state)
    {
        case Root::Done:            return "Done";
        case Root::Options:         return "Options";
        case Root::OptionsDone:     return "Options Done";
        case Root::Payload:         return "Payload";
    }
}

namespace experimental {

const char* get_description(Option::State state)
{
    switch(state)
    {
        case Option::FirstByte:
            return "Inspecting initial option data";

        case Option::OptionDeltaDone:
            return "Delta (option number) found";

        case Option::Payload:
            return "Payload marker found";

        case Option::OptionDelta:
            return "Delta found";

        case Option::OptionLengthDone:
            return "Option length compltely found";

        case Option::OptionValue:
            return "Inspecting value data";

        case Option::OptionValueDone:
            return "Value data complete";

        case Option::OptionDeltaAndLengthDone:
            return "Delta and length simultaneously done";

        default:
            return NULLPTR;
    }
}

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
std::ostream& operator <<(std::ostream& out, Option::State state)
{
    const char* description = get_description(state);

    if(description != NULLPTR)
        out << description;
    else
        out << (int)state;

    return out;
}
#endif

}


}
}
