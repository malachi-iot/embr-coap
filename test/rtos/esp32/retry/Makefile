# First crack at using ESTD tools to help out
# not working well at all so far
PROJECT_NAME := retry

EXTRA_CPPFLAGS := -DESP32

ROOT_PATH := ../../../..
EXT_PATH := ${ROOT_PATH}/ext
ESTDLIB_DIR := $(abspath $(EXT_PATH)/estdlib)
EXTRA_COMPONENT_DIRS := $(ESTDLIB_DIR)/tools/esp-idf

include $(ESTDLIB_DIR)/tools/esp-idf/version_finder.mk

include $(IDF_PATH)/make/project.mk