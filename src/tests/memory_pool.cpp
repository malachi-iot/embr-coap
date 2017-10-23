#include <catch.hpp>

#include "../MemoryPool.h"

using namespace moducom::dynamic;

TEST_CASE("Low-level memory pool tests", "[mempool_low]")
{
    WHEN("Index1 memory pool")
    {
        MemoryPool<> pool;

        int handle = pool.allocate(100);

        REQUIRE(pool.get_free() == (32 * (128 - 1) - 128));
    }
    WHEN("Index2 memory pool")
    {
        MemoryPool<> pool(IMemoryPool::Indexed2);

        int handle = pool.allocate(100);

        REQUIRE(pool.get_free() == (32 * (128 - 1) - 128));
    }
}