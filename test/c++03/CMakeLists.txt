cmake_minimum_required(VERSION 2.8)

#set(CMAKE_CXX_STANDARD 98)

project(coap-unity-tests)

set(UNITY_TESTS_DIR ../unity)

add_subdirectory(${UNITY_TESTS_DIR} unity-tests)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} coap-unity-tests-lib)