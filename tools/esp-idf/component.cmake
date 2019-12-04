set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

# app_update because we query runtime version in esp-idf/observer.h
set(COMPONENT_REQUIRES estdlib embr app_update)

set(COMPONENT_SRCS 
    ${ROOT_DIR}/src/coap.cpp
    ${ROOT_DIR}/src/coap/option-decoder.cpp
    ${ROOT_DIR}/src/coap-encoder.cpp

    ${ROOT_DIR}/src/coap/platform/esp-idf/observer.cpp
    )

# NOTE: moducom-memory is pure legacy at this point, just need to phase it out

set(COMPONENT_ADD_INCLUDEDIRS 
    ${ROOT_DIR}/src
    ${ROOT_DIR}/ext/moducom-memory/src
    )

register_component()