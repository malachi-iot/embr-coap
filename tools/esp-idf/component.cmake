set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(SRC_DIR ${ROOT_DIR}/src)
get_filename_component(SRC_DIR ${SRC_DIR} ABSOLUTE)

include(${SRC_DIR}/source.cmake)

message(DEBUG "mc-coap: SRC_DIR=${SRC_DIR}")

# app_update because we query runtime version in esp-idf/observer.h
set(COMPONENT_REQUIRES estdlib embr app_update)

list(TRANSFORM SOURCE_FILES PREPEND ${SRC_DIR}/)

set(COMPONENT_SRCS 
    ${SOURCE_FILES}
    ${ROOT_DIR}/src/coap/platform/esp-idf/observer.cpp
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    ${ROOT_DIR}/src
    )

register_component()
