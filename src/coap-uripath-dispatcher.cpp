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
    int result = factory.create(option_value_part, context);
}

}

}}