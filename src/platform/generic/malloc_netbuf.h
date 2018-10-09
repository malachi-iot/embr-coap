#pragma once

#include "mc/netbuf.h"
#include "mc/mem/platform.h"
#include <forward_list>


// diverging from existing NetBufDynamicMemory since that one isn't smart enough to
// manage multiple buffers yet.  This just leans on standard C++ all the way
namespace moducom { namespace coap {

// MemoryChunk* specific flavor
class ProcessedMemoryChunkBaseExperimental
{
protected:
    typedef pipeline::MemoryChunk chunk_t;

    chunk_t* m_chunk;
    typedef size_t size_type;
    size_type pos;

public:
    ProcessedMemoryChunkBaseExperimental() : pos(0) {}

    ProcessedMemoryChunkBaseExperimental(const ProcessedMemoryChunkBaseExperimental& copy_from) :
        m_chunk(copy_from.m_chunk),
        pos(copy_from.pos)
    {

    }

    const chunk_t& chunk() const { return *m_chunk; }

    chunk_t& chunk() { return *m_chunk; }

    // this retrieves processed data
    // to access processed data, go through chunk()
    uint8_t* unprocessed() { return m_chunk->data(pos); }

    const uint8_t* processed() const { return m_chunk->data(); }

    size_type length_unprocessed() const { return m_chunk->length() - pos; }

    size_type length_processed() const { return pos; }

    // ala std::string and std::vector, this indicates how much data
    // total can be stuffed in to this memory chunk
    size_type capacity() const { return m_chunk->length(); }

    // TODO: Put in bounds check here
    // potentially most visible and distinctive feature of ProcessedMemoryChunk is this
    // call and its "side affect"
    bool advance(size_type size)
    {
        pos += size;
        return true;
    }
};

class NetBufDynamicExperimental : public ProcessedMemoryChunkBaseExperimental
{
    typedef ProcessedMemoryChunkBaseExperimental base_t;
    typedef std::forward_list<pipeline::MemoryChunk> list_t;

    list_t chunks;
    // 'it' represents one step past current chunk
    list_t::iterator it;

    void allocate()
    {

    }

    void _first()
    {
        it = chunks.begin();
    }

public:
    bool end() const
    {
        return it == chunks.end();
    }

    bool next()
    {
        if(end()) return false;

        m_chunk = &(*it++);

        return true;
    }


    void first()
    {
        _first();

        m_chunk = &(*it++);
    }

    void written()
    {
        // indicates we are finished with our writing to fill up the netbuf
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    NetBufDynamicExperimental(NetBufDynamicExperimental&& move_from) :
        base_t(move_from),
        chunks(std::move(move_from.chunks)),
        it(std::move(move_from.it))
    {
    }
#endif


    // FIX: Careful because array allocation for inline alloc
    // results in all these buffers all preallocating right away
    // shouldn't stop our testing but not acceptable for production
    NetBufDynamicExperimental()
    {
        _first();
        chunk_t mem1((uint8_t*)malloc(4096), 4096);
        chunks.push_front(mem1);
        m_chunk = &chunks.front();
    }

    ~NetBufDynamicExperimental()
    {
        // Be advised this gets called during some kind of preinit, probably datapump explicitly allocating it
        _first();
        while(it != chunks.end())
        {
            free(it->data());
            it++;
        }
    }
};

}}
