# This is the project CMakeLists.txt file for the test subproject 
cmake_minimum_required(VERSION 3.5)

include(../setenv.cmake)

set(EMBR_COAP_DIR ${ROOT_DIR})

# Set the components to include the tests for.
# This can be overriden from CMake cache:
# - when invoking CMake directly: cmake -D TEST_COMPONENTS="xxxxx" ..
# - when using idf.py: idf.py build -T xxxxx
#
set(TEST_COMPONENTS "testable" CACHE STRING "List of components to test")

# NOTE: Can't make sense of "register_component" here.  Hmm.
#include(${EMBR_COAP_DIR}/tools/esp-idf/component.cmake NO_POLICY_SCOPE)

project(embr-coap-unity)