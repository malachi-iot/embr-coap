#include "coap.h"
//#include "mc/memory.h"
#if FEATURE_EMBR_COAP_SUBCONTEXT
#include "coap/context/sub.h"
#endif

namespace embr {
namespace coap {

const char* get_description(Option::Numbers number)
{
    switch(number)
    {
        case Option::Zeroed:        return "Zeroed";
        case Option::UriHost:       return "Uri-Host";
        case Option::ETag:          return "ETag";
        case Option::Observe:       return "Observe";
        case Option::UriPort:       return "Uri-Port";
        case Option::UriPath:       return "Uri-Path";
        case Option::ContentFormat: return "Content-Format";
        case Option::UriQuery:      return "Uri-Query";
        case Option::Accept:        return "Accept";
        case Option::Block1:        return "Block1";
        case Option::Block2:        return "Block2";
        case Option::ProxyUri:      return "Proxy-Uri";
        case Option::Echo:          return "Echo";

        default:                    return NULLPTR;
    }
}


const char* get_description(internal::Root::State state)
{
    using namespace internal;

    switch(state)
    {
        case Root::Done:            return "Done";
        case Root::Header:          return "Header";
        case Root::HeaderDone:      return "Header Done";
        case Root::Token:           return "Token";
        case Root::TokenStart:      return "Token Start";
        case Root::TokenDone:       return "Token Done";
        case Root::OptionsStart:    return "Options Start";
        case Root::Options:         return "Options";
        case Root::OptionsDone:     return "Options Done";
        case Root::Payload:         return "Payload";
        case Root::PayloadDone:     return "Payload Done";
        default:                    return NULLPTR;
    }
}

const char* get_description(header::Types type)
{
    switch(type)
    {
        case header::Types::Acknowledgement:    return "ACK";
        case header::Types::Confirmable:        return "CON";
        case header::Types::NonConfirmable:     return "NON";
        case header::Types::Reset:              return "RST";
        default:                                return NULLPTR;
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

namespace internal { inline namespace v1 {

#if FEATURE_EMBR_COAP_SUBCONTEXT
// DEBT: this helper function doesn't feel like it belongs in this
// catch-all coap.cpp
internal::v1::SubcontextBase::query split(const event::option& e)
{
    const estd::string_view sv = e.string();
    const estd::size_t split_pos = sv.find('=');

    if(split_pos == estd::string_view::npos)
        return { };

    const estd::string_view key(sv.substr(0, split_pos)),
        value(sv.substr(split_pos + 1));

    return { key, value };
}
#endif

}}

}
}


const char* to_string(embr::coap::Header::Code code)
{
    using Code = embr::coap::Header::Code;

    switch(code)
    {
        case Code::BadRequest:  return "Bad Request";
        case Code::Content:     return "Content";
        case Code::Created:     return "Created";
        case Code::Empty:       return "Empty";
        case Code::Get:         return "Get";
        case Code::Ping:        return "Ping";
        case Code::Pong:        return "Pong";
        case Code::Put:         return "Put";
        case Code::Valid:       return "Valid";

        default:    return "N/A";
    }
}