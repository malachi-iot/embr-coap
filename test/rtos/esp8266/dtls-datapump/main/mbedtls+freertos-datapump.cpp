#include <exp/datapump.hpp>
#include <platform/mbedtls/dtls-datapump.h>

// Experimental
// trying to 'prime' the LDF pump so it picks up our mbedtls-custom
#include "mbedtls/net.h"

// FIX: temporarily conflating datapump and ping responder
//#include <simple-ping-responder.hpp>

#ifdef ESP8266 // pio flavor (older SDK)
// for heap size info
#include "esp_common.h"
#elif defined(IDF_VER) // newer direct RTOS-SDK inclusion
#include "esp_system.h"
#endif

using namespace moducom::coap;

#define DIAGNOSTIC_MODE

extern "C" void coap_daemon(void *)
{
    Dtls dtls;

    int ret = dtls.setup();
    bool _continue = true;

    while(ret == 0 && _continue)
    {
        _continue = dtls.loop(&ret);

#ifdef DIAGNOSTIC_MODE
        // diagnostic mode, starts right back up again after an error
        if(!_continue || ret != 0)
        {
            printf("diagnostic mode: resuming\n");
            
            dtls.error(ret);
            dtls.shutdown();
#if defined(ESP8266) || defined(IDF_VER)
            printf("free heap size %d line %d \n", system_get_free_heap_size(), __LINE__);
#endif
            ret = dtls.setup();
            _continue = true;
        }
#endif
        //simple_ping_responder(ldh, datapump);
    }

    dtls.error(ret);
    dtls.shutdown();

    for(;;)
    {

    }
}