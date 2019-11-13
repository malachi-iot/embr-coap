set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

# NOTE: moducom-memory is pure legacy at this point, just need to phase it out

set(COMPONENT_ADD_INCLUDEDIRS 
    ${ROOT_DIR}/src
    ${ROOT_DIR}/ext/moducom-memory/src
    )

register_component()