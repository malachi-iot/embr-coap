#include <estd/type_traits.h>

#include <coap/context.h>

namespace embr { namespace coap {

template <class TContext>
inline typename estd::enable_if<
    estd::is_base_of<internal::ExtraContext, TContext>::value, bool>
    ::type
verify(TContext& context, bool condition, Header::Code fail_code)
{
    if(context.response_code.has_value())
    {
        Header::Code code = context.response_code.value();

        // short circuit, if we already have an error code, no need
        // to further verify condition
        if(!code.success()) return false;
    }

    if(condition == true) return true;

    context.response_code = fail_code;
    return false;
}


template <class TContext>
inline typename estd::enable_if<
    estd::is_base_of<tags::header_context, TContext>::value &&
    estd::is_base_of<internal::ExtraContext, TContext>::value, bool>
    ::type
verify(TContext& context, Header::Code code,
    Header::Code fail_code = Header::Code::BadRequest)
{
    return verify(context, context.header().code() == code, fail_code);
}


}}