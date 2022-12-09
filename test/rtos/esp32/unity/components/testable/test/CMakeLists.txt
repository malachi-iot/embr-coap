set(COMPONENT_SRCDIRS "." ${MCCOAP_DIR}/test/unity)
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_REQUIRES estdlib embr esp_timer mc-coap unity testable)

register_component()
