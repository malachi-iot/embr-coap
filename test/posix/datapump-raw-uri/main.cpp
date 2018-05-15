#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/decoder/subject.h>

#include "simple-uri-responder.hpp"

//using namespace std;
using namespace moducom::coap;

int main()
{

    std::cout << "Hello World!" << std::endl;
    std::cout << "Test: " << Option::Numbers::Block2 << std::endl;
    std::cout << "Test2: " << std::is_function<
            //decltype(moducom::coap::get_description)
            decltype(moducom::coap::get_description(Option::Numbers::UriPath))
            //decltype(main)
          >::value << std::endl;

    SocketsDatapumpHelper sdh;

    for(;;)
    {
        sdh.loop(sockets_datapump);

        sockets_datapump.service(simple_uri_responder, true);
    }

    return 0;
}
