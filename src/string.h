//
// Created by malachi on 10/20/17.
//

#ifndef SRC_STRING_H
#define SRC_STRING_H

#include "memory.h"
#include <string.h>

namespace moducom { namespace std {

// replacement for std::string class which instead uses handle-based memory
class string {
    typedef coap::Memory memory_t;

    memory_t::handle_t handle;
    memory_t& pool;

public:
    string(const char* src, memory_t& pool = memory_t::default_pool) : pool(pool)
    {
        handle = pool.allocate(strlen(src) + 1);
    }
};

}}
#endif //SRC_STRING_H
