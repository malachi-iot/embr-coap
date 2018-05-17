#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/decoder/subject.h>

#include "observable-raw-responder.hpp"

#include <chrono>

//using namespace std;
using namespace moducom::coap;

int main()
{

    std::cout << "Hello World!" << std::endl;

    SocketsDatapumpHelper sdh;

    for(;;)
    {
        sdh.loop(sockets_datapump);

        sockets_datapump.service(simple_observable_responder, true);
        //now = std::chrono::duration_cast<std::chrono::milliseconds>(
          //      std::chrono::system_clock::now());
        //evaluate_emit_observe(sockets_datapump, now);
    }

    return 0;
}
