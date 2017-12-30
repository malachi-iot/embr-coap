//
// Created by Malachi Burke on 12/24/17.
//

#ifndef MC_COAP_TEST_MEMORY_CHUNK_H
#define MC_COAP_TEST_MEMORY_CHUNK_H

// for size_t
#include <stddef.h>

namespace moducom { namespace pipeline {

namespace experimental {

// remember this is experimental, not sure we really want to commit to using this down the line
struct memory_chunk_traits
{
    // true = length field represents entire size of buffer, even if only a portion of it is used
    // false = length field represents portion of buffer
    static bool is_length_absolute() { return true; }
};

}

template <typename custom_size_t = size_t, class TTraits = experimental::memory_chunk_traits>
class MemoryChunkBase
{
protected:
public:
    typedef TTraits traits_t;

    custom_size_t length;

    // FIX: Temporary name until we refactor all usages away from raw field
    custom_size_t _length() const { return length; }

    void _length(custom_size_t l) { length = l; }
};

// TODO: Split out naming for MemoryChunk and something like MemoryBuffer
// MemoryBuffer always includes data* but may or may not be responsible for actual buffer itself
// MemoryChunk always includes buffer too - and perhaps (havent decided yet) may not necessarily include data*
class MemoryChunk : public MemoryChunkBase<>
{
public:
    uint8_t* data;

    // FIX: Temporary name until we refactor all usages away from raw field
    const uint8_t* _data() const { return data; }

    MemoryChunk(uint8_t* data, size_t length) : data(data)
    {
        this->length = length;
    }

    MemoryChunk() {}

    inline void set(uint8_t c, size_t length) { ::memset(data, c, length); }

    inline void set(uint8_t c) { set(c, length); }

    inline void memcpy(const uint8_t* copy_from, size_t length)
    {
        ::memcpy(data, copy_from, length);
    }

    inline uint8_t& operator[](size_t index) const
    {
        return data[index];
    }

    // generate a new MemoryChunk which is a subset of the current one
    // may be more trouble than it's worth calling this function
    inline MemoryChunk carve_experimental(size_t pos, size_t len) const { return MemoryChunk(data + pos, len); }

    // generate a new MemoryChunk with just the remainder of data starting
    // at pos
    inline MemoryChunk remainder(size_t pos) const
    {
        return MemoryChunk(data + pos, length - pos);
    }

    // Would prefer memorychunk itself to be more constant, perhaps we can
    // have a "ProcessedMemoryChunk" which includes a pos in it... ? or maybe
    // instead a ConstMemoryChunk just for those occasions..
    void advance_experimental(size_t pos) { data += pos; length -= pos; }
};

namespace layer1 {

template <size_t buffer_length>
class MemoryChunk
{
    uint8_t buffer[buffer_length];

protected:
    uint8_t* writable_data_experimental(size_t offset = 0) { return buffer + offset; }


public:
    size_t length() const { return buffer_length; }
    const uint8_t* data(size_t offset = 0) const { return buffer + offset; }

    inline void memcpy(const uint8_t* copy_from, size_t length)
    {
        ::memcpy(buffer, copy_from, length);
    }


    inline void copy_from(const pipeline::MemoryChunk& chunk)
    {
        ::memcpy(buffer, chunk._data(), chunk._length());
    }


    inline void set(uint8_t c) { ::memset(buffer, c, buffer_length); }

    inline int compare(const void* compare_against, size_t length)
    {
        return ::memcmp(buffer, compare_against, length);
    }

    inline uint8_t& operator[](size_t index)
    {
        return buffer[index];
    }
};

}

namespace layer2 {
// Variant of layer2.  buffer pointer NOT used, but size field IS used
template<size_t buffer_length, typename custom_size_t = size_t>
class MemoryChunk :
        public MemoryChunkBase<custom_size_t>,
        public layer1::MemoryChunk<buffer_length>
{
    typedef layer1::MemoryChunk<buffer_length> base_t;

public:
    inline custom_size_t length() const { return MemoryChunkBase<custom_size_t>::length; }

    inline int compare(const void* compare_against)
    {
        return base_t::compare(compare_against, length());
    }
};

}

namespace layer3 {
template<size_t buffer_length>
class MemoryChunk : public pipeline::MemoryChunk
{
    uint8_t buffer[buffer_length];

public:
    MemoryChunk() : pipeline::MemoryChunk(buffer, buffer_length) {}

    inline void memset(uint8_t c) { ::memset(buffer, c, buffer_length); }
};

}


class PipelineMessage : public MemoryChunk
{
    // One might think of Status vs CopiedStatus as WriteStatus vs ReadStatus
public:
    // Expected this will live on a stack or instance field somewhere
    // With this we can retrieve how our message is progressing through
    // the pipelines
    struct Status
    {
    private:
        // message has reached its final destination
        bool _delivered : 1;
        // message has been copied and original no longer
        // needed
        bool _copied : 1;

        // 6 bits of things just for me!
        // TODO: Consider a flag for memory handle
        // TODO: Consider also a cheap-n-nasty RTTI for extended Statuses
        // TODO: Consider a demarcation flag to denote a packet boundary.  This gets a little tricky since
        //       some scenarios have many different boundaries to consider
        uint8_t reserved: 6;
    public:
        bool delivered() const { return _delivered; }

        void delivered(bool value) { _delivered = value; }

        bool copied() const { return _copied; }

        void copied(bool value) { _copied = value; }

        // different from regular copy in that only the message itself has been copied
        // used primarily for by-reference messages vs by-value messages.  Not sure if
        // a) we're gonna even use by-ref messages and b) if we do whether this will be
        // useful
        bool experimental_shallow_copied() const { return false; }

        // 8 bits of things just for you!
        uint8_t user : 8;

        // Consider an extended version of this with a semaphore
    };

    // this is like regular Status except it's merely copied along with each message,
    // thus partially alleviating memory management issue with regular Status*
    // naturally, because of this, copied_status can't be used to report status back to
    // the originator
    struct CopiedStatus
    {
        // // 8 bits of things just for you!
        // note in buffered situations, only the last user status is carried.  This contradicts
        // boundary behavior described below, so plan accordingly when using boundaries
        uint32_t user : 8;

        // boundary indicates a signal to a pipeline consumer (the one doing reads) that
        // possibly an action needs to be taken (boundary reached).  End of chunk is the boundary
        // boundaries tend to be application specific, so use this for boundary types 1-7, 0 being
        // no boundary.  Only restriction is that in buffered situations, larger boundary types will "eat"
        // smaller ones until buffer is read back out again:
        //
        // ...1...1...2...3...1...2...1
        //                ^       ^
        // buffer read ---+     --+
        //
        // becomes
        // ...............3.......2...
        uint32_t boundary : 3;

        // Helper flag to indicate whether get_buffer was utilized, in which case
        // we should take extra measures to avoid buffer allocation and/or copying
        uint32_t is_preferred_experimental : 1;
    } copied_status;

    Status* status;

    PipelineMessage() : status(NULLPTR) {}

    PipelineMessage(uint8_t* data, size_t length) : status(NULLPTR),
                                                    MemoryChunk(data, length)
    {
        copied_status.user = 0;
        copied_status.boundary = 0;
    }

    PipelineMessage(const MemoryChunk& chunk) : MemoryChunk(chunk), status(NULLPTR) {}

};

}}

#endif //MC_COAP_TEST_MEMORY_CHUNK_H
