#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/decoder/subject.h>

#include "simple-cbor-responder.hpp"

//using namespace std;
using namespace moducom::coap;

int main()
{
    std::cout << "Hello World!" << std::endl;

    SocketsDatapumpHelper sdh;

    for(;;)
    {
        sdh.loop(sockets_datapump);

        sockets_datapump.service(simple_cbor_responder, true);
    }

    return 0;
}
