//
// Created by malachi on 2/25/18.
//

#pragma once

#include "../coap-token.h"
#include "../coap-header.h"

namespace moducom { namespace coap {


class ContextBase
{
    typedef layer2::Token token_t;
    token_t* _token;

public:
    ContextBase() : _token(NULLPTR) {}

    // if a) incoming message has a token and b) we've decoded the token and have it
    // available, then this will be non-null
    token_t* token() const { return _token; }

    // Since tokens arrive piecemeal and oftentimes not at all, we have a non-constructor
    // based setter
    void token(token_t* token) { _token = token; }
};

// New-generation request context, replacement for premature coap_transmission one
// Incoming request handlers/dispatchers may extend this context, but this is a good
// foundational base class
// Called Incoming context because remember some incoming messages are RESPONSES, even
// when we are the server (ACKs, etc)
class IncomingContext : ContextBase
{
    Header _header;

public:
    IncomingContext()
    {
        // always zero out, signifying UNINITIALIZED so that querying
        // parties don't mistakenly think we have a header
        _header.raw = 0;
    }

    // if incoming message has had its header decoded, then this is valid
    Header& header() { return _header; }

    bool have_header() const { return _header.version() > 0; }
};

}}