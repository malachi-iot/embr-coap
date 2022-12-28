set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
get_filename_component(ROOT_DIR ${ROOT_DIR} ABSOLUTE)

add_subdirectory(${ROOT_DIR}/ext/embr/test/rpi/pico/support support)
add_subdirectory(${ROOT_DIR}/src coap)
