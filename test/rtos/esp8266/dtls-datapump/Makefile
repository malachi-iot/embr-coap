#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := dtls-datapump

# Needed up here for global coverage
EXTRA_CPPFLAGS := -DFEATURE_MC_MEM_LWIP -DFEATURE_MCCOAP_MBEDTLS

# this irritates ESP8266_RTOS_SDK
#EXTRA_CPPFLAGS += -DMBEDTLS_CONFIG_FILE="custom_mbedtls.h"

include $(IDF_PATH)/make/project.mk