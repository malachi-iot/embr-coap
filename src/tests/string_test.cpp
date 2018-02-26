
#include <catch.hpp>

#include "MemoryPool.h"
#include "mc/string.h"

using namespace moducom::dynamic;

TEST_CASE("String tests 2", "[strings-2]")
{
    SECTION("Section 1")
    {
        moducom::std::string str = "Hi";
        char temp[128];

        str.populate(temp);

        REQUIRE(str == temp);
    }
}
