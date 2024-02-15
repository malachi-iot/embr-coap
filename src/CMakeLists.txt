cmake_minimum_required(VERSION 3.6)

project(embr-coap VERSION 0.9.1)

#set(CMAKE_CXX_STANDARD 11)
# NOTE: Need to make this set externally and well tested somehow
# set(CMAKE_CXX_STANDARD 98)

include(source.cmake)

# DEBT: Treating these basically as interface libraries.  Formalize that
if(NOT TARGET estd)
    add_subdirectory(${ESTD_DIR}/src estd)
endif()
if(NOT TARGET embr)
    add_subdirectory(${EMBR_DIR}/src embr)
endif()

configure_file(../tools/cmake/version.in.h ${CMAKE_CURRENT_LIST_DIR}/coap/version.h)
set(SOURCE_FILES ${SOURCE_FILES})

# DEBT: doc files not showing up in CLion (acting like a noop here)
add_library(${PROJECT_NAME} ${SOURCE_FILES} ${DOC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC .)

target_link_libraries(${PROJECT_NAME} estd embr)

add_library(embr::coap ALIAS ${PROJECT_NAME})
add_library(mc-coap ALIAS ${PROJECT_NAME})
