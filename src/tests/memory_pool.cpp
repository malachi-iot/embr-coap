
#include <catch.hpp>

#include "../MemoryPool.h"
#include "../mc/memory-pool.h"
#include "../coap-token.h"

using namespace moducom::dynamic;

// Using TestToken because I am not convinced I want to embed "is_allocated" into layer2::Token
// somewhat harmless, but confusing out of context (what does is_allocated *mean* if token is
// never used with the pool code)
class TestToken : public moducom::coap::layer2::Token
{
public:
    TestToken() {}

    TestToken(const moducom::pipeline::MemoryChunk::readonly_t& chunk)
    {

        ASSERT_ERROR(true, chunk.length() <= 8, "chunk.length <= 8");
        copy_from(chunk);
    }

    ~TestToken() { length(0); }

    bool is_active() const { return length() > 0; }
};

TEST_CASE("Low-level memory pool tests", "[mempool-low]")
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
    SECTION("Traditional memory pool")
    {
        PoolBase<TestToken, 8> pool;
        TestToken hardcoded;

        hardcoded.set("1234", 4);

        REQUIRE(pool.count() == 0);

        TestToken& allocated = pool.allocate(hardcoded);

        REQUIRE(pool.count() == 1);

        pool.free(allocated);

        REQUIRE(pool.count() == 0);
    }
}
