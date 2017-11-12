//
// Created by Malachi Burke on 11/12/17.
//

#include "pipeline.h"

using namespace moducom::pipeline;

namespace moducom { namespace pipeline {

namespace layer3 { namespace experimental {

bool BufferProviderPipeline::get_buffer(MemoryChunk **memory_chunk)
{
    return false;
}

MemoryChunk BufferProviderPipeline::advance(size_t size, PipelineMessage::CopiedStatus status)
{
    MemoryChunk chunk;

    chunk.data = NULLPTR;

    return chunk;
}

}}

}}