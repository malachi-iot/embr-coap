//
// Created by malachi on 5/30/18.
//

#ifndef MC_COAP_TEST_MBEDTLS_DATAPUMP_H
#define MC_COAP_TEST_MBEDTLS_DATAPUMP_H

#include <stdint.h> // for uint8_t and friends

// NOTE: Potentially interacts with a transport-specific datapump
// TODO: Following suit of DataPump itself, we might move this down into mcmem also
namespace moducom { namespace coap {

// TODO: Might have to go dtls-datapump.hpp and make this templated on TNetBuf
class Dtls
{
    // as indicated by dtls_server.c examples
    // TODO: In order for 'observe' functionality we'll need to be able to construct these
    // from scratch.  Standard request/response stuff we should be good echoing back what
    // we got on input
    typedef uint8_t addr_t[16];

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
