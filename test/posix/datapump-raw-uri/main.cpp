#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/decoder/subject.h>
#include <mc/memory-chunk.h>

using namespace std;
using namespace moducom::coap;

typedef NetBufDynamicExperimental netbuf_t;

int main()
{
    cout << "Hello World!" << endl;

    SocketsDatapumpHelper sdh;

    for(;;)
    {
        sdh.loop();
    }

    return 0;
}
