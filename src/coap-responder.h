//
// Created by malachi on 11/15/17.
//

#pragma once

#include "coap_transmission.h"
#include "coap-generator.h"
#include "coap_daemon.h"

namespace moducom { namespace coap {


class GeneratorResponder;

// Needs a better name.  Represents CoAP messages pushed instead of responded to
// such as (sort of) subscription messages
class Pusher
{
    friend class GeneratorResponder;

    Pusher* next;

public:
    Pusher() : next(NULLPTR) {}

    virtual bool push(IPipeline& output);
};

class GeneratorResponder : public CoAP::IResponder
{
    CoAPGenerator generator;
    pipeline::IPipeline& output;

    // NOTE: may want to track pushers out in daemon instead of responder
    Pusher* pusher_head;
    Pusher* pusher_tail;

public:
    GeneratorResponder(pipeline::IPipeline& output) :
        output(output),
        pusher_head(NULLPTR),
        pusher_tail(NULLPTR),
        generator(output)
    {}

    void add(Pusher* pusher)
    {
        if(pusher_head == NULLPTR)
        {
            pusher_head = pusher;
            pusher_tail = pusher;
            return;
        }

        pusher_tail->next = pusher;
    }

    bool process_iterate();
};

}}


