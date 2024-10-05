set(COMPONENT_SRCDIRS ${EMBR_COAP_DIR}/test/unity)
#set(COMPONENT_SRCDIRS "." ${MCCOAP_DIR}/test/unity)
#set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_REQUIRES estdlib embr esp_timer embr-coap unity testable)

register_component()
