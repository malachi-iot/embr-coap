#pragma once

namespace embr { namespace coap {

namespace tags {

// DEBT: Fake stand in for c++20 concept.  These tags promise presence of certain methods
// document specifics for each tag
struct token_context {};
struct header_context {};
struct address_context {};
struct incoming_context {};

}

}}
