//
// Created by Malachi Burke on 1/6/18.
//

#include "experimental.h"
#include "experimental-factory.h"

namespace moducom { namespace coap { namespace experimental {



// input must be a CON message for this method
bool process_reset(Header input, Header* output)
{
    output->copy_message_id(input);
    output->type(Header::Reset);
    return true;
}

const pipeline::MemoryChunk v2::ManagedBuffer::current() const
{
    chunk_t temp((uint8_t*)chunk.data(current_pos), chunk.length() - current_pos);

    return temp;
}

const pipeline::MemoryChunk::readonly_t v2::ManagedBuffer::current_ro(boundary_t boundary) const
{
    if(boundary == 0)
    {
        ro_chunk_t temp((const uint8_t*) chunk.data(current_pos), chunk.length() - current_pos);

        return temp;
    }
    else
    {
        const BoundaryDescriptor* bd = &boundaries[current_boundary];
        size_t temp_length;
        int i = current_boundary;

        for(; i < boundary_count && bd->boundary != boundary; i++, bd++)
        {
        }

        if(i == boundary_count)
        {
            // got to end and didn't find boundary, so use whole length
            temp_length = chunk.length();
        }
        else
        {
            // found the boundary
            temp_length = bd->pos;
        }

        temp_length -= current_pos;

        ro_chunk_t temp((const uint8_t*) chunk.data(current_pos), temp_length);

        return temp;
    }
}


bool v2::ManagedBuffer::next()
{
    // this is for read mode only
    const BoundaryDescriptor& bd = boundaries[current_boundary];

    current_pos += bd.pos;

    if(current_boundary < boundary_count)
    {
        current_boundary++;
    }
    /*
    {
        BoundaryDescriptor& bd = boundaries[current_boundary];
    } */

    return true;
}


bool v2::ManagedBuffer::reset(bool reset_boundaries)
{
    current_pos = 0;
    current_boundary = 0;
    return true;
}


bool v2::ManagedBuffer::boundary(boundary_t boundary, size_t position)
{
    if(boundary != 0)
    {
        BoundaryDescriptor& bd = boundaries[boundary_count++];

        bd.boundary = boundary;
        bd.pos = current_pos + position;
    }

    current_pos += position;
    return true;
}


v2::ManagedBuffer::ManagedBuffer() :
        boundary_count(0),
        current_boundary(0),
        current_pos(0)
{

}



}}}