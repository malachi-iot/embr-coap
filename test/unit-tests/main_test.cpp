//
// Created by malachi on 10/20/17.
//

#ifdef _MSC_BUILD
#define CATCH_CONFIG_RUNNER
#else
#define CATCH_CONFIG_MAIN
#endif

#include <iostream>
#include "MemoryPool.h"

#ifdef UNUSED
// https://github.com/philsquared/Catch/issues/531
std::ostream& operator<<(std::ostream& os, moducom::std::string& value)
{
    moducom::std::string::auto_ptr_t s(value);

    os << s;

    return os;
}

#include <catch.hpp>

namespace Catch {

template<> struct StringMaker<moducom::std::string>
{
    static std::string convert(moducom::std::string const& value)
    {
        moducom::std::string::auto_ptr_t s((moducom::std::string&) value);
        
        return std::string(s);
    }
};

}

#define STATIC_STR1  "Test1"
#define STATIC_STR2  "Followed by this"

TEST_CASE("String tests", "[strings]")
{
    moducom::std::string str(STATIC_STR1);

    REQUIRE(str == STATIC_STR1);

    moducom::std::string str2 = str + STATIC_STR2;

    REQUIRE(str2 == STATIC_STR1 STATIC_STR2);

    int test_size = sizeof(moducom::dynamic::MemoryPoolIndexedHandlePage);
    test_size = sizeof(moducom::dynamic::MemoryPoolDescriptor);

    str2 += " ";
    str2 += 4;

    REQUIRE(str2 == STATIC_STR1 STATIC_STR2 " 4");

    moducom::std::string str3 = str2;

    REQUIRE(str3 == str2);
}
#else
#include <catch.hpp>
#endif


// So that Visual Studio doesn't immediately close the window
#ifdef CATCH_CONFIG_RUNNER

int main(int argc, char* const argv[])
{
    int result = Catch::Session().run(argc, argv);

    std::cin.get();

    return result;
}

#endif