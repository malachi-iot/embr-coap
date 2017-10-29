#include "mc/memory.h"
#include <stdlib.h>
#include <string.h>

namespace moducom { namespace dynamic {

Memory Memory::default_pool;

Memory::handle_t Memory::allocate(size_t size)
{
    return (handle_t) malloc(size);
}

Memory::handle_t Memory::allocate(const void* data, size_t size, size_t size_copy)
{
    void* new_data = malloc(size);

    // TODO: do some sanity checks make sure size_copy isn't too big
    if(size_copy != 0)
        memcpy(new_data, data, size_copy);
    else
        memcpy(new_data, data, size);

    return (handle_t) new_data;
}

Memory::handle_t Memory::copy(handle_t handle, size_t size, size_t size_copy)
{
    return allocate((void*)handle, size, size_copy);
}


bool Memory::free(handle_t handle)
{
    ::free((void*) handle);
    return true;
}

}}
