cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 98)
#set(CMAKE_CXX_STANDARD 11)

# DEBT: This is not always true for C++98 scenarios, but implicitly true for ADSPBFIN (inttypes.h isn't
# available, but it seems to have those defs) and most other compilers I use
add_compile_definitions(FEATURE_STD_INTTYPES)

project(coap-unity-tests)

set(UNITY_TESTS_DIR ../unity)

add_subdirectory(${UNITY_TESTS_DIR} unity-tests)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} coap-unity-tests-lib)