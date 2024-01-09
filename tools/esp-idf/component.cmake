get_filename_component(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
set(SRC_DIR ${ROOT_DIR}/src)

include(${SRC_DIR}/source.cmake)

message(DEBUG "embr::coap: SRC_DIR=${SRC_DIR}")

# app_update because we query runtime version in esp-idf/observer.h
set(COMPONENT_REQUIRES estdlib embr app_update)

list(TRANSFORM SOURCE_FILES PREPEND ${SRC_DIR}/)

set(COMPONENT_SRCS ${SOURCE_FILES})

set(COMPONENT_ADD_INCLUDEDIRS 
    ${ROOT_DIR}/src
    )

register_component()
