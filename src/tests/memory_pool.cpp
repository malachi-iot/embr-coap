#include <catch.hpp>

#include "../MemoryPool.h"

using namespace moducom::dynamic;

TEST_CASE("Low-level memory pool tests", "[mempool_low]")
{
    MemoryPool<> pool;

    int handle = pool.allocate(100);

    REQUIRE(pool.get_free() == (32 * (128 - 1) - 128));
}