cmake_minimum_required(VERSION 3.8)
project(src)

#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 98)

set(SOURCE_FILES main.cpp coap.cpp coap.h memory.cpp memory.h string.cpp string.h)
add_executable(src ${SOURCE_FILES})