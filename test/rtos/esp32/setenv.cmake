set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# DEBT: Upgrade estd itself to propagate compiler defines system-wide
include(${CMAKE_CURRENT_LIST_DIR}/components/estdlib/version_finder.cmake)