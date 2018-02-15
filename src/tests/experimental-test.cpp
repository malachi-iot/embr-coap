#include <catch.hpp>

#include "../coap-decoder.h"
#include "test-data.h"
#include "../mc/experimental.h"
//#include "../mc/pipeline.h"
#include "../mc/experimental-factory.h"

using namespace moducom::coap;
//using namespace moducom::pipeline;

int test(experimental::FnFactoryContext context)
{

}


TEST_CASE("experimental tests", "[experimental]")
{
    typedef moducom::pipeline::MemoryChunk chunk_t;
    typedef chunk_t::readonly_t ro_chunk_t;
    SECTION("layer1")
    {
        experimental::layer1::ProcessedMemoryChunk<128> chunk;

        chunk[0] = 1;
        chunk.processed(1);
    }
    SECTION("layer2")
    {
        experimental::layer2::ProcessedMemoryChunk<128> chunk;

        chunk[0] = 1;
        chunk.processed(1);
    }
    SECTION("option traits")
    {
        // it appears pre-C++11 lets us use regular const for this.  That's handy
        CONSTEXPR Option::Numbers option = Option::UriPath;
        CONSTEXPR Option::Numbers option2 = Option::Size1;

        typedef experimental::OptionTrait<option> option_trait_t;
        typedef experimental::OptionTrait<option2> option2_trait_t; // this line compiles, but nothing can really use option2_trait_t

        REQUIRE(Option::String == option_trait_t::format());
        //REQUIRE(Option::UInt == option2_trait_t::format());
    }
    SECTION("ManagedBuffer")
    {
        experimental::v2::ManagedBuffer buffer;

        moducom::pipeline::MemoryChunk c = buffer.current();

        c[0] = 'a';
        c[1] = 'b';
        c[2] = 'c';

        buffer.boundary(1, 3);

        c = buffer.current();

        c[0] = 'A';
        c[1] = 'B';
        c[2] = 'C';
        c[2] = 'D';

        buffer.boundary(1, 4);
        buffer.boundary(2, 0);

        buffer.reset();

        moducom::pipeline::MemoryChunk::readonly_t c_ro = buffer.current_ro(1);

        REQUIRE(c_ro[0] == 'a');
        REQUIRE(c_ro.length() == 3);

        buffer.next();
        c_ro = buffer.current_ro(1);

        REQUIRE(c_ro[0] == 'A');
        REQUIRE(c_ro.length() == 4);

        buffer.reset();

        c_ro = buffer.current_ro(2);

        REQUIRE(c_ro.length() == 7);
    }
    SECTION("More ManagedBuffer")
    {
        experimental::v2::ManagedBuffer buffer;

        chunk_t w = buffer.current();

        w[0] = '1';
        w[1] = '2';

        buffer.boundary(2, 2);

        w = buffer.current();

        w[0] = 'a';
        w[1] = 'b';
        w[2] = 'c';

        buffer.boundary(2, 3);

        buffer.reset();

        ro_chunk_t r = buffer.read(2);

        REQUIRE(r[0] == '1');
        REQUIRE(r[1] == '2');
        REQUIRE(r.length() == 2);

        r = buffer.read(2);

        REQUIRE(r[0] == 'a');
        REQUIRE(r.length() == 3);

    }
    SECTION("FnFactory")
    {
        experimental::FnFactoryItem<const char*, int> items[] =
        {
            experimental::factory_item_helper("fred", test)
        };
        //experimental::factory_helper(items);

        experimental::FnFactory<const char*, int> factory(items);

        experimental::FnFactoryContext context;

        int result = factory.create("test", context);

        REQUIRE(result == -1);
    }
}