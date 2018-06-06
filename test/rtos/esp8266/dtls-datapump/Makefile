#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := dtls-datapump

# Needed up here for global coverage
EXTRA_CPPFLAGS := -DFEATURE_MC_MEM_LWIP -DFEATURE_MCCOAP_MBEDTLS

# this irritates ESP8266_RTOS_SDK, as it already sets this in
#  components/ssl/Makefile.projbuild
# EXTRA_CPPFLAGS += -DMBEDTLS_CONFIG_FILE="mbedtls/custom_mbedtls.h"

# experimental, try to pick up our local mbedtls/esp_config or sdkconfig.h file
#  doesn't work, damn
COMPONENT_ADD_INCLUDEDIRS := main

include $(IDF_PATH)/make/project.mk