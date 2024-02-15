include(${CMAKE_CURRENT_LIST_DIR}/../../../tools/esp-idf/setvars.cmake)

set(EXTRA_COMPONENT_DIRS
    "${ROOT_DIR}/tools/esp-idf/components"
    "${EMBR_DIR}/tools/esp-idf/components"
    "${EMBR_DIR}/test/rtos/esp-idf/components"  # embr helper
    "${ESTD_DIR}/tools/esp-idf/components"
    )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# DEBT: Upgrade estd itself to propagate compiler defines system-wide
include(${ESTD_DIR}/tools/esp-idf/version_finder.cmake)