cmake_minimum_required(VERSION 3.6)

project(embr-coap-test)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
    message(STATUS "${PROJECT_NAME}: Defaulting c++=11")
endif()

add_definitions(-DUNIT_TESTING)
# NOTE: Normally this would go into an 'opts' but 'mc/opts.h', while a good idea, is no longer
# ubiquitous
#add_definitions(-DDEBUG)

include(../../tools/cmake/setvars.cmake)

Set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
    GIT_TAG         v2.13.10
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

FetchContent_MakeAvailable(Catch2)

# this must come first so that it picks up proper opts.h file
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(${ROOT_DIR}/src coap)

set(SOURCE_FILES
        main_test.cpp coap.cpp
        coap_lowlevel.cpp
        cbor-test.cpp

        coap-decoder-test.cpp
        coap-header-test.cpp
        test-data.h coap-encoder-test.cpp
        coap-token-test.cpp
        coap-core.cpp
        coap-observable-test.cpp
        blockwise-test.cpp
        context-test.cpp

        # Disabled as part of mc-mem removal.  Contains DataPump which is crusty but usable -
        # its netbuf naming is a misnomer, really it can be just about any buffer - IIRC
        experimental2-test.cpp

        decoder-subject-test.cpp
        mc/opts.h

        test-observer.h

        retry-test.cpp

        uint-test.cpp
        )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} embr::coap embr Catch2)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Makes concepts-related failures easier to read
    target_compile_options(${PROJECT_NAME} PUBLIC -fconcepts-diagnostics-depth=2)
endif()