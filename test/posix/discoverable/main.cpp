#include <iostream>

#include <platform/posix/sockets-datapump.h>

using namespace moducom::coap;

int main()
{
    std::cout << "CoRE test" << std::endl;

    SocketsDatapumpHelper sdh;

    for(;;)
    {
        sdh.loop(sockets_datapump);

        auto& datapump = sockets_datapump;

        /*
        typedef TDataPump datapump_t;
        typedef typename datapump_t::netbuf_t netbuf_t;
        typedef typename datapump_t::addr_t addr_t;
        typedef typename datapump_t::Item item_t;

        f(!datapump.dequeue_empty())
        {
            item_t& item = datapump.dequeue_front();

            // hold on to ipaddr for when we enqueue a response
            addr_t ipaddr = item.addr();

            //decoder_t decoder(*sdh.front(&ipaddr, datapump));
            decoder_t decoder(*item.netbuf());

            layer2::Token token; */
    }

    return 0;
}