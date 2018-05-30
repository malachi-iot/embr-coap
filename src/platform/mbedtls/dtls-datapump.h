//
// Created by malachi on 5/30/18.
//

#ifndef MC_COAP_TEST_MBEDTLS_DATAPUMP_H
#define MC_COAP_TEST_MBEDTLS_DATAPUMP_H

// NOTE: Potentially interacts with a transport-specific datapump
// TODO: Following suit of DataPump itself, we might move this down into mcmem also
namespace moducom { namespace coap {

void dtls_setup();
void dtls_loop();

}}

#endif //MC_COAP_TEST_MBEDTLS_DATAPUMP_H
