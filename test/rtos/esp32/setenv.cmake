set(EXT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../ext)
set(EXTRA_COMPONENT_DIRS
    "${EXT_DIR}/embr/tools/esp-idf/components"
    "${EXT_DIR}/embr/test/rtos/esp-idf/components"
    "${CMAKE_CURRENT_LIST_DIR}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# DEBT: Upgrade estd itself to propagate compiler defines system-wide
include(${CMAKE_CURRENT_LIST_DIR}/components/estdlib/version_finder.cmake)
