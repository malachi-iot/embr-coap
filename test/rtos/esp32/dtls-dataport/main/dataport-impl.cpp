#include <embr/dataport.hpp>
#include <platform/mbedtls/dtls.hpp>

extern "C" void test_task(void*)
{
    mbedtls::DtlsServer server;

    server.load_certs();
    server.setup();

    for(;;)
    {
        bool open;

        server.handshake();

        do
        {
            // a read is generally set up as nonblocking and is the only mechanism
            // i've spotted to let us know a connection got closed
            //open = server.read();
            open = true;

            // is_pending has many uses, but for DTLS we use it to see if another datagram
            // is on the heels of any we just read.  However, it seems a little clumsy
            // since we always have to call .read() anyway to check for a closed connection
            bool is_pending = server.is_pending();
        } 
        while(open);
    }
}