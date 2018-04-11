#include <coap.h>
#include <mc/memory-chunk.h>
#include <sys/socket.h>
#include "main.h"

#include "coap-observable.h"

using namespace moducom::pipeline;

template <>
size_t service_coap_in(const struct sockaddr_in& address_in, MemoryChunk& inbuf, MemoryChunk& outbuf)
{
    return 0;
}


// return 0 always, for now
template <>
size_t service_coap_out(struct sockaddr_in* address_out, MemoryChunk& outbuf)
{
    return 0;
}
