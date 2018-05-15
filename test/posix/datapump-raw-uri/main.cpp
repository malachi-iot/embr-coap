#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/decoder/subject.h>

#include "simple-uri-responder.hpp"

using namespace std;
using namespace moducom::coap;

typedef NetBufDynamicExperimental netbuf_t;

int main()
{

    cout << "Hello World!" << endl;
    cout << "Test: " << Option::Numbers::Block2 << endl;
    cout << "Test2: " << std::is_function<
            //decltype(moducom::coap::get_description)
            decltype(moducom::coap::get_description(Option::Numbers::UriPath))
            //decltype(main)
          >::value << endl;

    SocketsDatapumpHelper sdh;

    for(;;)
    {
        // FIX: Strangely, the ping responder works OK btu this seemingly identical code
        // blasts through poll() with no data and gets a 'Bad file descriptor' on the recvfrom call
        // Thinking it might be related to how we do our netbuf allocations? Not sure
        sdh.loop(sockets_datapump);

        //simple_uri_responder(sdh, sockets_datapump);

        sockets_datapump.service(simple_uri_responder2, true);
    }

    return 0;
}
