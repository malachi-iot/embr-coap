#include <coap.h>
#include <mc/memory-chunk.h>

using namespace moducom::pipeline;

// TODO: Will need a push-style service_coap_out as well

void service_coap_in(MemoryChunk& inbuf, MemoryChunk& outbuf)
{

}


// return 0 always, for now
size_t service_coap_out(MemoryChunk& outbuf)
{
    return 0;
}
