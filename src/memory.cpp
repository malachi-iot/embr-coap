//
// Created by malachi on 10/20/17.
//

#include "memory.h"
#include <malloc.h>
#include <string.h>

namespace moducom { namespace coap {

Memory Memory::default_pool;

Memory::handle_t Memory::allocate(size_t size)
{
    return malloc(size);
}

Memory::handle_t Memory::allocate(const void* data, size_t size)
{
    void* new_data = malloc(size);

    memcpy(new_data, data, size);
}


bool Memory::free(handle_t handle)
{
    ::free(handle);
    return true;
}

}}