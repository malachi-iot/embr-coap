# FIX: This seems wrong; instead there should be a component-level indication of
# what folderss to include
ROOT_PATH := ../../../../..
EXT_PATH := ${ROOT_PATH}/ext
#CPPFLAGS := -DFEATURE_MCCOAP_MBEDTLS 
CPPFLAGS := -DESP32 ${CPPFLAGS}
COMPONENT_SRCDIRS += ${ROOT_PATH}/src ${ROOT_PATH}/src/platform/mbedtls
COMPONENT_PRIV_INCLUDEDIRS := ${ROOT_PATH}/src ${EXT_PATH}/estdlib/src ${EXT_PATH}/moducom-memory/src
