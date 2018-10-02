#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/decoder/subject.h>
#include <mc/memory-chunk.h>

using namespace std;
using namespace moducom::coap;

#include "simple-ping-responder.hpp"

typedef NetBufDynamicExperimental netbuf_t;

int main()
{
    cout << "Hello World!" << endl;

    SocketsDatapumpHelper sdh;

    sockets_datapump_t datapump;

    for(;;)
    {
        sdh.loop(datapump);

        simple_ping_responder(datapump);
    }

    return 0;
}
