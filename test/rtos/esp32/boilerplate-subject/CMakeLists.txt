# This is the project CMakeLists.txt file for the test subproject 
cmake_minimum_required(VERSION 3.5)

# Include the components directory of the main application:
#
set(EXTRA_COMPONENT_DIRS "../components")

set(ESTDLIB_DIR ../../../../ext/estdlib)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include(${ESTDLIB_DIR}/tools/esp-idf/version_finder.cmake)

project(coap-boilerplate-subject)
