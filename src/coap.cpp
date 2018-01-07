#include "coap.h"
#include "mc/memory.h"

namespace moducom {
namespace coap {

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

#ifdef DEBUG2
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
