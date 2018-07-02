cmake_minimum_required(VERSION 3.6)

project(observable-test)

set(CMAKE_CXX_STANDARD 11)

set(MC_ROOT ../../..)
set(MC_MEM_DIR ${MC_ROOT}/ext/moducom-memory/src)
set(ESTD_DIR ${MC_ROOT}/ext/estdlib/src)
set(MC_COAP_DIR ${MC_ROOT}/src)

include_directories(${ESTD_DIR})
include_directories(${MC_MEM_DIR})
include_directories(${MC_COAP_DIR})

# remember to put this down here so that mc/opts gets properly discovered
add_subdirectory(${MC_COAP_DIR} coap)

set(SOURCE_FILES
        observable.cpp
	main.cpp)

add_executable(observable-test ${SOURCE_FILES})

target_link_libraries(observable-test moducom-coap-lib)
