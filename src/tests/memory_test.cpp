
#include <catch.hpp>

#include "MemoryPool.h"
#include "mc/string.h"

// Not really using it, and isn't behaving on macOS
//#include <malloc.h>

using namespace moducom::dynamic;

TEST_CASE("General memory tests", "[memory]")
{
    SECTION("Free/not free checks")
    {
        // just dynamically allocating this one for fun
        IMemory* memory = new Memory();

        IMemory::handle_opaque_t handle = memory->allocate("Hi", 10, 3);

        char* value = (char*) memory->lock(handle);

        REQUIRE(strcmp(value, "Hi") == 0);

        memory->unlock(handle);

#ifdef __GNUC__
//        struct mallinfo mi = mallinfo();
#endif

        // FIX: no destructor is gonna get called cuz not yet a virtual
        delete memory;
    }
}
