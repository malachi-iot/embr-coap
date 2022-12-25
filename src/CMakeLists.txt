cmake_minimum_required(VERSION 3.6)

project(mc-coap)

#set(CMAKE_CXX_STANDARD 11)
# NOTE: Need to make this set externally and well tested somehow
# set(CMAKE_CXX_STANDARD 98)

include(source.cmake)

# DEBT: Treating these basically as interface libraries.  Formalize that
set(ESTDLIB_DIR ${ROOT_DIR}/ext/estdlib/src)
set(EMBR_DIR ${ROOT_DIR}/ext/embr/src)

include_directories(${ESTDLIB_DIR})
include_directories(${EMBR_DIR})

set(SOURCE_FILES ${SOURCE_FILES})

# DEBT: doc files not showing up in CLion (acting like a noop here)
add_library(${PROJECT_NAME} ${SOURCE_FILES} ${DOC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC .)
