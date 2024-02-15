cmake_minimum_required(VERSION 3.17)

project(coap-unity-tests-lib)

add_compile_definitions(FEATURE_MCCOAP_LEGACY_EVENTS=0)

include(../../tools/cmake/setvars.cmake)

# FIX: Broken, we get this through fetchcontent now
set(UNITY_DIR ${ESTD_DIR}/ext/Unity)
set(COAP_DIR ${ROOT_DIR}/src)

if(NOT TARGET embr::coap)
    add_subdirectory(${COAP_DIR} coap)
endif()

add_subdirectory(${UNITY_DIR} unity)

add_library(${PROJECT_NAME} 
    basic.cpp
    context.cpp
    decoder.cpp
    encoder.cpp
    experimental.cpp
    retry.cpp
    rfc7641.cpp
    uri.cpp)

add_library(mc-coap::unity-tests ALIAS ${PROJECT_NAME})
#add_library(embr::coap::unity-tests ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC unity embr embr::coap)

target_include_directories(${PROJECT_NAME} PUBLIC ${COAP_DIR})
