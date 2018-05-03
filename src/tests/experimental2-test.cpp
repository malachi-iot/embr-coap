#include <catch.hpp>

#include <exp/datapump.hpp>
#include <exp/retry.h>

using namespace moducom::coap::experimental;

TEST_CASE("experimental 2 tests")
{
    SECTION("A")
    {
        int fakenetbuf = 5;
        uint8_t fakeaddr[4];

        Retry<int> retry;

        retry.enqueue(fakeaddr, fakenetbuf);

        // Not yet, need newer estdlib first with cleaned up iterators
        //Retry<int>::Item* test = retry.front();
    }
}