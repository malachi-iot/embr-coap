//
// Created by malachi on 10/20/17.
//

#define CATCH_CONFIG_MAIN
#include <iostream>
#include "../string.h"
#include "../MemoryPool.h"

// https://github.com/philsquared/Catch/issues/531
std::ostream& operator<<(std::ostream& os, moducom::std::string& value)
{
    moducom::std::string::auto_ptr_t s(value);

    os << s;

    return os;
}

namespace Catch {

std::string toString(moducom::std::string &value)
{
    moducom::std::string::auto_ptr_t s(value);

    return std::string(s);
}
}

#include <catch.hpp>

#define STATIC_STR1  "Test1"
#define STATIC_STR2  "Followed by this"

TEST_CASE("String tests", "[strings]")
{
    moducom::std::string str(STATIC_STR1);

    REQUIRE(str == STATIC_STR1);

    moducom::std::string str2 = str + STATIC_STR2;

    REQUIRE(str2 == STATIC_STR1 STATIC_STR2);

    int test_size = sizeof(moducom::coap::MemoryPoolIndexedHandlePage);
    test_size = sizeof(moducom::coap::MemoryPoolDescriptor);

    str2 += " ";
    str2 += 4;

    REQUIRE(str2 == STATIC_STR1 STATIC_STR2 " 4");

    moducom::coap::IMemoryPool* pool = new moducom::coap::MemoryPool<>();
}