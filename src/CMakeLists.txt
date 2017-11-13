cmake_minimum_required(VERSION 3.8)
project(mc-coap-test)

#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 98)

include_directories(../ext/Catch/include)

set(SOURCE_FILES main.cpp coap.cpp coap.h memory.cpp mc/memory.h string.cpp mc/string.h tests/main_test.cpp MemoryPool.cpp MemoryPool.h tests/memory_pool.cpp tests/coap.cpp coap_transmission.cpp coap_transmission.h tests/coap_lowlevel.cpp coap_subscription.cpp coap_subscription.h cbor.cpp tests/coap_send.cpp mc/memory_index2.h mc/memory_pool.h mc/memory_index.h tests/string_test.cpp tests/memory_test.cpp coap_daemon.cpp coap_daemon.h tests/coap_daemon.cpp mc/pipeline.h tests/pipeline_test.cpp mc/pipeline.cpp coap-generator.h coap-generator.cpp)
add_executable(src ${SOURCE_FILES})
