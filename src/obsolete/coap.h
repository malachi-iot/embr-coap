#pragma once

namespace embr { namespace coap { namespace experimental {

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
std::ostream& operator <<(std::ostream& out, Option::State state);
#endif

}}}


#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
#ifdef __ADSPBLACKFIN__
// Blackfin has an abbreviated version of ostream
inline std::ostream& operator <<(std::ostream& os, const embr::coap::Option::Numbers& v)
#else
template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator <<(std::basic_ostream<CharT, Traits>& os, const embr::coap::Option::Numbers& v)
#endif
{
    const char* d = embr::coap::get_description(v);

    if(d)
        os << d;
    else
        os << (int)v;

    return os;
}


#ifdef FEATURE_CPP_DECLVAL
// valiant attempt, but doesn't work
// is_function always evaluates 'false' - something to do with specifying get_description's arguments
template <class CharT, class Traits, typename THasGetDescription
          ,
          typename std::enable_if<
              std::is_function<
                    decltype(embr::coap::get_description(std::declval<THasGetDescription>()))
                  >::value
              >::type
          >
std::basic_ostream<CharT, Traits>& operator <<(std::basic_ostream<CharT, Traits>& os, const THasGetDescription& t)
{
    const char* d = embr::coap::get_description(t);

    if(d)
        os << d;
    else
        ::operator <<(os, t); // pretty sure this isn't going to work

    return os;
}
#endif

#endif