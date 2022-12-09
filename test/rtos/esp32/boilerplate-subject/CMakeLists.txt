# This is the project CMakeLists.txt file for the test subproject 
cmake_minimum_required(VERSION 3.5)

include (../setenv.cmake)

project(coap-boilerplate-app)

# 02MAY22 MB - No .su files getting generated that I can find
idf_build_set_property(COMPILE_OPTIONS "-fstack-usage" APPEND)
# Makes zero difference to stack size
#idf_build_set_property(COMPILE_OPTIONS "-DFEATURE_ESTD_CHARTRAITS=1" APPEND)

