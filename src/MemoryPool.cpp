//
// Created by malachi on 10/21/17.
//

#include "MemoryPool.h"

namespace moducom { namespace dynamic {

void MemoryPoolIndexedHandlePage::initialize(uint8_t free_page, uint8_t page_count)
{
    // TODO: refactor to make size 0 based, to increase usable space
    header.size = 1;
    header.tier = IMemory::Indexed;
    header.followup = false;
    // set up initial fully-empty handle
    handle_t& handle = indexedHandle[0];
    handle.allocated = false;
    // maximum size is page_count - 1 since we're using one page for
    // system operations
    handle.size = page_count;
    handle.page = free_page; // skip by first page since that's sys_page
}

MemoryPoolIndexedHandlePage::handle_t* MemoryPoolIndexedHandlePage::get_first_unallocated_handle(
        size_t total,
        handle_opaque_t* index)
{
    for(int i = 0; i < header.size; i++)
    {
        handle_t& descriptor = indexedHandle[i];

        if(!descriptor.allocated && descriptor.is_initialized())
        {
            *index = i;
            return &descriptor;
        }
    }

    return NULLPTR;
}



}}
