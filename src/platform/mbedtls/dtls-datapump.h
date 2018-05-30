//
// Created by malachi on 5/30/18.
//

#ifndef MC_COAP_TEST_MBEDTLS_DATAPUMP_H
#define MC_COAP_TEST_MBEDTLS_DATAPUMP_H

// NOTE: Potentially interacts with a transport-specific datapump
// TODO: Following suit of DataPump itself, we might move this down into mcmem also
namespace moducom { namespace coap {

class Dtls
{
public:
    int setup();    // returns 0 on success. if fail, go straight to shutdown
    // true = loop again, false = stop, exit, *ret holds return code, can not be null
    bool loop(int* ret);
    void error(int);
    void shutdown();
    void shutdown(int ret)
    {
        error(ret);
        shutdown();
    }
};


}}

#endif //MC_COAP_TEST_MBEDTLS_DATAPUMP_H
