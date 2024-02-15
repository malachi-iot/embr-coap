# DEBT: Consolidate with ../cmake version of this
get_filename_component(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)

set(EMBR_DIR ${ROOT_DIR}/ext/embr)
set(ESTD_DIR ${EMBR_DIR}/ext/estdlib)

# Old name
set(ESTDLIB_DIR ${ESTDLIB_DIR})
