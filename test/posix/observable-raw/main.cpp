#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/decoder/subject.h>

#include "observable-raw-responder.hpp"

#include <chrono>


//using namespace std;
using namespace moducom::coap;

layer1::ObservableRegistrar<sockets_datapump_t::addr_t, 10> observable_registrar;


int main()
{

    std::cout << "Hello World!" << std::endl;

    SocketsDatapumpHelper sdh;

    auto start = std::chrono::system_clock::now();

    for(;;)
    {
        sdh.loop(sockets_datapump);

        sockets_datapump.service(simple_observable_responder, observable_registrar, true);

        auto now = std::chrono::system_clock::now();

        std::chrono::milliseconds now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

        evaluate_emit_observe(sockets_datapump, observable_registrar, now_ms);
    }

    return 0;
}
