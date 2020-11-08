cmake_minimum_required(VERSION 2.8)

project(coap-unity-tests-lib)

set(ROOT_DIR ${PROJECT_SOURCE_DIR}/../..)
set(ESTDLIB_DIR ${ROOT_DIR}/ext/estdlib)
set(UNITY_DIR ${ESTDLIB_DIR}/ext/Unity)
set(COAP_DIR ${ROOT_DIR}/src)
set(MC_MEM_DIR ${ROOT_DIR}/ext/moducom-memory)

# DEBT: eliminate estdlib ref here since COaP project should pull this in
add_subdirectory(${ESTDLIB_DIR}/src estdlib)
add_subdirectory(${COAP_DIR} coap)
add_subdirectory(${UNITY_DIR} unity)

#include_directories(${ESTDLIB_DIR} PUBLIC)
#include_directories(${UNITY_DIR} PUBLIC)

add_library(${PROJECT_NAME} 
    basic.cpp)

target_link_libraries(${PROJECT_NAME} unity estd_lib)

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${ESTDLIB_DIR}/src
    ${UNITY_DIR}
    .
    )
target_include_directories(${PROJECT_NAME} PUBLIC ${COAP_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${MC_MEM_DIR}/src)
