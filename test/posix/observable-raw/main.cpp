#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/decoder/subject.h>

#include "observable-raw-responder.hpp"

#include <chrono>


//using namespace std;
using namespace moducom::coap;

#ifdef EXP
moducom::coap::ObservableRegistrar<
        estd::layer1::vector<
                moducom::coap::ObservableSession<sockets_datapump_t::addr_t>, 10
                >
        > observable_registrar;
#endif


int main()
{

    std::cout << "Hello World!" << std::endl;

    SocketsDatapumpHelper sdh;

    auto start = std::chrono::system_clock::now();

    for(;;)
    {
        sdh.loop(sockets_datapump);

        sockets_datapump_t::addr_t addr_in;

        // VERY cheezy way to get responder address for observer service
        if(!sockets_datapump.dequeue_empty())
        {
            std::clog << "Got IP = " << addr_in.sin_addr.s_addr << std::endl;
            sockets_datapump.dequeue_in(&addr_in);
        }

#ifdef EXP
        sockets_datapump.service(simple_observable_responder, observable_registrar, true);
#else
        sockets_datapump.service(simple_observable_responder, true);
#endif

        auto now = std::chrono::system_clock::now();

        std::chrono::milliseconds now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

#ifdef EXP
        evaluate_emit_observe(sockets_datapump, observable_registrar, now_ms);
#else
        evaluate_emit_observe(sockets_datapump, addr_in, now_ms);
#endif
    }

    return 0;
}
