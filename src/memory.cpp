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

Memory::handle_t Memory::allocate(const void* data, size_t size, size_t size_copy)
{
    void* new_data = malloc(size);

    // TODO: do some sanity checks make sure size_copy isn't too big
    if(size_copy != 0)
        memcpy(new_data, data, size_copy);
    else
        memcpy(new_data, data, size);
}

Memory::handle_t Memory::copy(handle_t handle, size_t size, size_t size_copy)
{
    return allocate(handle, size, size_copy);
}


void Memory::resize(handle_t handle, size_t new_size)
{
    handle = realloc(handle, new_size);
}

bool Memory::free(handle_t handle)
{
    ::free(handle);
    return true;
}

}}