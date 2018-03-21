//
// Created by malachi on 2/14/18.
//

#include "coap-uripath-dispatcher.h"

namespace moducom { namespace coap {

namespace experimental {

void UriDispatcherHandler::on_option(number_t number,
                                     const pipeline::MemoryChunk::readonly_t& option_value_part,
                                     bool last_chunk)
{
    // should only arrive here as a result of cascading observers/handlers,
    // so typically (but not always) either see UriPath here
    // FIX: this is a little shaky here, it might be reasonable to expect we *only*
    // ever arrive here either on exactly to-be-evaluated URIPATH *or* on "Always"
    // since we already have a handler
    if(number == Option::UriPath)
    {
        if(handler == NULLPTR)
        {
            // context passing in as a reference, so objstack modifications are going to persist
            // NOTE: does not work with partial data
            handler = factory.create(option_value_part, context);

            // if we successfully find a handler (match a URI) then from here on out
            // we are always the conduit to that handler
            if(handler != NULLPTR)
                // This signals to parent dispatcher to not deallocate our memory
                this->interested(base_t::Always);
            else
                // otherwise, a non-matched URI means we (this particular UriDispatcherHandler)
                // does not service the incoming URI path ever
                this->interested(base_t::Never);
        }
    }


    if (handler != NULLPTR)
    {
        handler->on_option(number, option_value_part, last_chunk);
    }
}


void UriDispatcherHandler::on_payload(
    const pipeline::MemoryChunk::readonly_t& payload_part,
    bool last_chunk)
{
    // NOTE: Shouldn't need to do NULLPTR check, should only come here when set
    // 'interested'
    if(handler != NULLPTR)
        handler->on_payload(payload_part, last_chunk);
}


#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
void UriDispatcherHandler::on_complete()
{
    // NOTE: Shouldn't need to do NULLPTR check, should only come here when set
    // 'interested'

    // NOTE: This currently assumes we're doing in-place memory management
    if (handler != NULLPTR)
    {
        handler->on_complete();
        handler->~IDispatcherHandler();
    }
}
#endif

}

}}
