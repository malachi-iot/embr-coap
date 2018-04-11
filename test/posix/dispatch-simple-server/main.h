#pragma once

template <class TAddress>
size_t service_coap_in(const TAddress& address_in, moducom::pipeline::MemoryChunk& in,
                     moducom::pipeline::MemoryChunk& out);

template <class TAddress>
size_t service_coap_out(TAddress* address_out, moducom::pipeline::MemoryChunk& out);


