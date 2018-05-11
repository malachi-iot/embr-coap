//
// Created by malachi on 12/29/17.
//

#ifndef MC_COAP_TEST_COAP_TOKEN_H
#define MC_COAP_TEST_COAP_TOKEN_H

#include <stdint.h>
#include <string.h>
#include "coap/platform.h"
#include "mc/memory-chunk.h"

#ifdef FEATURE_CPP_INITIALIZER_LIST
#include <initializer_list>
#include <algorithm> // for std::copy
#endif

namespace moducom { namespace coap {

namespace layer1 {

// https://tools.ietf.org/html/rfc7252#section-5.3.1
// semi-duplicate of one in coap_transmission
class Token : public moducom::pipeline::layer1::MemoryChunk<8>
{
    typedef moducom::pipeline::layer1::MemoryChunk<8> base_t;

public:
#ifdef FEATURE_CPP_INITIALIZER_LIST
    Token(std::initializer_list<uint8_t> l)
    {
        // TODO: Move this actually into memorychunk itself, and make
        // this constructor just pass it along (if defaults don't handle it)
        std::copy(l.begin(), l.end(), base_t::buffer);
    }

    Token() {}
#endif
};

}

namespace layer2 {

// https://tools.ietf.org/html/rfc7252#section-5.3.1
// semi-duplicate of one in coap_transmission
class Token : public moducom::pipeline::layer2::MemoryChunk<8, uint8_t>
{
    typedef moducom::pipeline::layer2::MemoryChunk<8, uint8_t> base_t;

public:
    Token() {}

    Token(const layer1::Token& t, size_t tkl)
    {
        memcpy(t.data(), tkl);
        length(tkl);
    }

    Token(const uint8_t* data, size_t tkl)
    {
        memcpy(data, tkl);
        length(tkl);
    }

    // TODO: put this into MemoryChunk itself
    // we don't expose ReadOnly constructor which can do this copy because
    // it might lose the const qualifiers
    operator moducom::pipeline::MemoryChunk::readonly_t() const
    {
        return moducom::pipeline::MemoryChunk::readonly_t(data(), length());
    }
};


}

class SimpleTokenGenerator
{
    uint32_t current;

public:
    SimpleTokenGenerator() : current(0) {}

    void generate(coap::layer2::Token* token);

    // FIX: Temporarily making setter always available until architecture settles down
//#ifdef DEBUG
    void set(uint32_t current) { this->current = current; }
//#endif
};



}}


#endif //MC_COAP_TEST_COAP_TOKEN_H
