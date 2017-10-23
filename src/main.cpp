#include <iostream>
#include "coap.h"
#include "memory.h"
#include "string.h"

int main_old() {
    std::cout << "Hello, World!" << std::endl;
    typedef moducom::dynamic::Memory memory_t;
    memory_t memory;

    memory_t::handle_t handle = memory.allocate(10);

    int* int_ptr = memory.lock<int>(handle);

    *int_ptr = 4;

    memory.unlock(handle);
    memory.free(handle);

    moducom::std::string str("Hello World");

    str += ": you nut";

    // copy constructor test
    moducom::std::string str2(str);

    moducom::std::string str3 = str2 + "!";

    char output[128];

    str3.populate(output);

    std::cout << "And again: " << output << std::endl;

    return 0;
}