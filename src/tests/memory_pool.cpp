
#include <catch.hpp>

#include "../MemoryPool.h"

using namespace moducom::dynamic;

TEST_CASE("Low-level memory pool tests", "[mempool_low]")
{
    SECTION("Index1 memory pool")
    {
        MemoryPool<> pool;

        int handle = pool.allocate(100);

        REQUIRE(pool.get_free() == (32 * (128 - 1) - 128));
    }
    SECTION("Index2 memory pool")
    {
        MemoryPool<> pool(IMemory::Indexed2);
        typedef MemoryPoolIndexed2HandlePage::handle_t handle_t;

        SECTION("Allocations")
        {
            int allocated_count = 1;
            int unallocated_count = 1;

            int handle = pool.allocate(100);

            REQUIRE(pool.get_allocated_handle_count() == allocated_count);

            allocated_count++;

            int handle2 = pool.allocate(100);

            REQUIRE(pool.get_allocated_handle_count() == allocated_count);
            REQUIRE(pool.get_allocated_handle_count(false) == unallocated_count);
            REQUIRE(pool.get_free() == 32 * (128 - 1) -
                                       allocated_count * (32 * 4) - // allocated

                                       // unallocated siphons a little memory off the top too, because a PageData
                                       // is being tracked in the page pool
                                       unallocated_count * sizeof(handle_t::PageData)
            );
        }
        SECTION("Locking")
        {
            REQUIRE(pool.get_allocated_handle_count() == 0);

            int handle = pool.allocate(100);

            REQUIRE(pool.get_allocated_handle_count() == 1);

            pool.lock(handle);
            pool.unlock(handle);

            REQUIRE(pool.get_allocated_handle_count() == 1);

            pool.free(handle);

            REQUIRE(pool.get_allocated_handle_count() == 0);
        }
    }
}
