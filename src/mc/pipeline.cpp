//
// Created by Malachi Burke on 11/12/17.
//

#include "pipeline.h"

using namespace moducom::pipeline;

namespace moducom { namespace pipeline {

namespace layer3 { namespace experimental {

bool BufferProviderPipeline::get_buffer(pipeline::MemoryChunk **memory_chunk)
{
    return false;
}

pipeline::MemoryChunk BufferProviderPipeline::advance(size_t size, PipelineMessage::CopiedStatus status)
{
    pipeline::MemoryChunk chunk;

    chunk.data(NULLPTR);

    return chunk;
}

}}

}}
