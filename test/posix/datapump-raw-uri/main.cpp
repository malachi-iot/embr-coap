#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/decoder/subject.h>
#include <mc/memory-chunk.h>

using namespace std;
using namespace moducom::coap;

#include "simple-uri-responder.hpp"

typedef NetBufDynamicExperimental netbuf_t;

int main()
{
    cout << "Hello World!" << endl;

    SocketsDatapumpHelper sdh;

    for(;;)
    {
        // FIX: Strangely, the ping responder works OK btu this seemingly identical code
        // blasts through poll() with no data and gets a 'Bad file descriptor' on the recvfrom call
        // Thinking it might be related to how we do our netbuf allocations? Not sure
        sdh.loop();

        simple_uri_responder(sdh, sockets_datapump);
    }

    return 0;
}
