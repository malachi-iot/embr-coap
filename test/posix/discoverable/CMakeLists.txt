cmake_minimum_required(VERSION 3.12)
project(discoverable)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_DIR ../../..)
set(EMBR_DIR ${ROOT_DIR}/ext/embr/src)
set(ESTD_DIR ${ROOT_DIR}/ext/estdlib/src)
set(COAP_DIR ${ROOT_DIR}/src)
set(MC_MEM_DIR ${ROOT_DIR}/ext/moducom-memory/src)

include_directories(${PROJECT_SOURCE_DIR})

include_directories(${COAP_DIR})
include_directories(${ESTD_DIR})
include_directories(${EMBR_DIR})
include_directories(${MC_MEM_DIR})

#add_subdirectory(${ESTD_DIR} estd)
#add_subdirectory(${EMBR_DIR} embr)
add_subdirectory(${COAP_DIR} coap)

add_executable(discoverable main.cpp mc/opts.h)

target_link_libraries(${PROJECT_NAME} moducom-coap-lib)