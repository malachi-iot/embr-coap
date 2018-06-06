#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_EMBED_TXTFILES := server_root_cert.pem

# FIX: This seems wrong; instead there should be a component-level indication of
# what folderss to include
COMPONENT_PRIV_INCLUDEDIRS := ../components/coap/main ../components/estdlib/main ../components/mc-mem/main

# -fno-threadsafe-statics -DESP_DEBUG -DFEATURE_MC_MEM_LWIP -DUDP_DEBUG=LWIP_DBG_ON
CXXFLAGS += -DFEATURE_MC_MEM_LWIP -DFEATURE_MCCOAP_MBEDTLS
# FIX: above I don't think is quite working as expected

#COMPONENTS += coap