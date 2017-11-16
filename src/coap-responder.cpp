//
// Created by malachi on 11/15/17.
//

#include "coap-responder.h"

namespace moducom { namespace coap {

bool GeneratorResponder::process_iterate()
{
    // if push code exists
    if(pusher_head)
    {
        // execute it iteratively
        if(!pusher_head->push(output)) return false;

        // if iterative execution of push head is complete, then advance to next
        // push if present
        pusher_head = pusher_head->next;
        if(pusher_head == NULLPTR) pusher_tail = NULLPTR;

        return true;
    }
}

}}