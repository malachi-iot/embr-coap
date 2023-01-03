//
// Created by malachi on 10/20/17.
//

#ifdef _MSC_BUILD
#define CATCH_CONFIG_RUNNER
#else
#define CATCH_CONFIG_MAIN
#endif

#include <iostream>

#include <catch2/catch.hpp>


// So that Visual Studio doesn't immediately close the window
#ifdef CATCH_CONFIG_RUNNER

int main(int argc, char* const argv[])
{
    int result = Catch::Session().run(argc, argv);

    std::cin.get();

    return result;
}

#endif