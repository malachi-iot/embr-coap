#include <catch.hpp>

#include "../MemoryPool.h"

using namespace moducom::coap;

TEST_CASE("Low-level memory pool tests", "[mempool-low]")
{
    MemoryPool<> pool;

    pool.allocate(100);
}