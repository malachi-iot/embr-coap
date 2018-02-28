cmake_minimum_required(VERSION 3.6)
project(simple_server)

set(CMAKE_CXX_STANDARD 11)

ADD_DEFINITIONS(-D__CPP11___)

set(MEM_LIB_PATH ../../ext/moducom-memory/src)
set(LIB_PATH ../../src)

file(GLOB MEM_LIB_SRC "${MEM_LIB_PATH}/*.cpp" "${MEM_LIB_PATH}/mc/*.cpp")

include_directories(${MEM_LIB_PATH})

# due to memory.h & string.h naming collisions
include_directories(${LIB_PATH})

file(GLOB LIB_SRC "${LIB_PATH}/*.cpp" "${LIB_PATH}/mc/*.cpp")

add_library(MY_LIB ${LIB_SRC} ${MEM_LIB_SRC})

set(SOURCE_FILES main.cpp)

add_executable(simple_server ${SOURCE_FILES})

target_link_libraries(simple_server MY_LIB)
