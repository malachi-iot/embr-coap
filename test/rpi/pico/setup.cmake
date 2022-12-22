set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
message(STATUS "GOT HERE: ${ROOT_DIR}")
get_filename_component(ROOT_DIR ${ROOT_DIR} ABSOLUTE)
set(TEMPLATE_DIR ${ROOT_DIR}/test/template)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# picks up estd for us also
add_subdirectory(${ROOT_DIR}/ext/embr/src embr)
# DEBT: coap lib doesn't truly include embr/estd - perhaps make it an interface
# library, because it implicitly is already
add_subdirectory(${ROOT_DIR}/src coap)
