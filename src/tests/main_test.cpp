//
// Created by malachi on 10/20/17.
//

#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "../string.h"

#define STATIC_STR1  "Test1"
#define STATIC_STR2  "Followed by this"

TEST_CASE("String tests", "[strings]")
{
    moducom::std::string str(STATIC_STR1);

    REQUIRE(str == STATIC_STR1);

    moducom::std::string str2 = str + STATIC_STR2;

    const char* test = str2.lock();

    //REQUIRE(str2 == STATIC_STR1 STATIC_STR2);

    str2.unlock();
}