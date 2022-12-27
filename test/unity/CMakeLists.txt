cmake_minimum_required(VERSION 2.8)

project(coap-unity-tests-lib)

add_compile_definitions(FEATURE_MCCOAP_LEGACY_EVENTS=0)

set(ROOT_DIR ${PROJECT_SOURCE_DIR}/../..)
set(ESTDLIB_DIR ${ROOT_DIR}/ext/estdlib)
set(EMBR_DIR ${ROOT_DIR}/ext/embr)
set(UNITY_DIR ${ESTDLIB_DIR}/ext/Unity)
set(COAP_DIR ${ROOT_DIR}/src)

add_subdirectory(${COAP_DIR} coap)
add_subdirectory(${UNITY_DIR} unity)

add_library(${PROJECT_NAME} 
    basic.cpp
    decoder.cpp
    encoder.cpp
    experimental.cpp
    retry.cpp
    rfc7641.cpp
    uri.cpp)

target_link_libraries(${PROJECT_NAME} unity embr mc-coap)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${ESTDLIB_DIR}/src
    ${EMBR_DIR}/src
    ${UNITY_DIR}
    .
    )
target_include_directories(${PROJECT_NAME} PUBLIC ${COAP_DIR})
