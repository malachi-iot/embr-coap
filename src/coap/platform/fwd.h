#pragma once

namespace embr { namespace coap { namespace sys_paths {


template <class TContext>
bool build_sys_reply(TContext& context, TContext::encoder_type& encoder);

template <class TContext>
bool send_sys_reply(Tcontext& context, TContext::encoder_type& encoder);

}}}
