//
// Created by malachi on 10/20/17.
//

#ifndef SRC_STRING_H
#define SRC_STRING_H

#include "memory.h"
#include <string.h>
#include <stdint.h>

namespace moducom { namespace std {

// replacement for std::string class which instead uses handle-based memory
class string {
    typedef coap::Memory memory_t;

    memory_t::SmartHandle handle;
    // TODO: optimize size out - perhaps smarthandle itself can track it?
    size_t size;

    // TODO: make this smart enough to not allocate memory until someone actually
    // changes the string
public:
    string(const char* src, memory_t& pool = memory_t::default_pool) :
            handle(pool.allocate(src, strlen(src) + 1), pool)
    {
    }

    // Do this differently for C++ with the && move technique
    string(const string& copy_from) : handle(copy_from.handle)
    {

    }

    // experimental.  Safer than leaving string locked open and
    // I imagine 95% of the time this is the operation the caller
    // ultimately wants also
    void populate(char* output)
    {
        char* input = handle.lock<char>();

        memcpy(output, input, size);
        output[size] = 0;

        handle.unlock();
    }

    string& operator += (const char* src)
    {
        size_t src_len = strlen(src);
        handle.resize(size + src_len + 1);
        char* dest = handle.lock<char>();
        memcpy(dest + size, src, src_len);
        size += src_len;
        handle.unlock();
        return *this;
    }

    inline string operator +(const char *src)
    {
        /*
        string new_str()
        size_t src_len = strlen(src);
        handle.resize(size + src_len + 1);
        */
    }
};

}}
#endif //SRC_STRING_H
