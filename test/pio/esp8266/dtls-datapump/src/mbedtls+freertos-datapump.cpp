#include <exp/datapump.hpp>
#include <platform/mbedtls/dtls-datapump.h>

// FIX: temporarily conflating datapump and ping responder
//#include <simple-ping-responder.hpp>

using namespace moducom::coap;

extern "C" void coap_daemon(void *)
{
    Dtls dtls;

    int ret = dtls.setup();
    bool _continue = true;

    while(ret == 0 && _continue)
    {
        _continue = dtls.loop(&ret);

        //simple_ping_responder(ldh, datapump);
    }

    dtls.error(ret);
    dtls.shutdown();

    for(;;)
    {

    }
}