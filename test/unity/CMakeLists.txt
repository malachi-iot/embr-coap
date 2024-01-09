cmake_minimum_required(VERSION 3.17)

project(coap-unity-tests-lib)

add_compile_definitions(FEATURE_MCCOAP_LEGACY_EVENTS=0)

set(ROOT_DIR ${PROJECT_SOURCE_DIR}/../..)
set(ESTDLIB_DIR ${ROOT_DIR}/ext/embr/ext/estdlib)
set(EMBR_DIR ${ROOT_DIR}/ext/embr)
set(UNITY_DIR ${ESTDLIB_DIR}/ext/Unity)
set(COAP_DIR ${ROOT_DIR}/src)

if(NOT TARGET mc-coap)
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

target_link_libraries(${PROJECT_NAME} unity embr mc-coap)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${ESTDLIB_DIR}/src
    ${EMBR_DIR}/src
    ${UNITY_DIR}
    .
    )
target_include_directories(${PROJECT_NAME} PUBLIC ${COAP_DIR})
