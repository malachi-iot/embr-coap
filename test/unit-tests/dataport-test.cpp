#include <catch.hpp>

#include <estd/exp/observer.h>

#include <exp/dataport.hpp>

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE

struct LoopbackTransport
{
    typedef moducom::io::experimental::NetBufDynamicMemory<> netbuf_t;
    typedef uint32_t addr_t;

    typedef LoopbackTransport transport_descriptor_t;

    template <class TDataPort>
    LoopbackTransport(TDataPort*, int dummy_ip_port) {}
};

TEST_CASE("Dataport tests", "[dataport]")
{
    SECTION("A")
    {
        auto s = estd::experimental::layer0::make_subject();

        auto dataport = experimental::make_dataport<LoopbackTransport>(s);
    }
}

#endif
