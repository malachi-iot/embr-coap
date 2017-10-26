cmake_minimum_required(VERSION 3.8)
project(mc-coap-test)

#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 98)

include_directories(../ext/Catch/include)

set(SOURCE_FILES main.cpp coap.cpp coap.h memory.cpp memory.h string.cpp string.h tests/main_test.cpp MemoryPool.cpp MemoryPool.h tests/memory_pool.cpp tests/coap.cpp coap_transmission.cpp coap_transmission.h)
add_executable(src ${SOURCE_FILES})