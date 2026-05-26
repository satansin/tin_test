# Build only the Qhull libraries required by tin_test (no apps, no CTest).
# QHULL_ROOT must point at a Qhull source tree (e.g. third_party/qhull).
if(NOT QHULL_ROOT)
  message(FATAL_ERROR "QHULL_ROOT is not set.")
endif()

set(_qhull_r_sources
  ${QHULL_ROOT}/src/libqhull_r/global_r.c
  ${QHULL_ROOT}/src/libqhull_r/stat_r.c
  ${QHULL_ROOT}/src/libqhull_r/geom2_r.c
  ${QHULL_ROOT}/src/libqhull_r/poly2_r.c
  ${QHULL_ROOT}/src/libqhull_r/merge_r.c
  ${QHULL_ROOT}/src/libqhull_r/libqhull_r.c
  ${QHULL_ROOT}/src/libqhull_r/geom_r.c
  ${QHULL_ROOT}/src/libqhull_r/poly_r.c
  ${QHULL_ROOT}/src/libqhull_r/qset_r.c
  ${QHULL_ROOT}/src/libqhull_r/mem_r.c
  ${QHULL_ROOT}/src/libqhull_r/random_r.c
  ${QHULL_ROOT}/src/libqhull_r/usermem_r.c
  ${QHULL_ROOT}/src/libqhull_r/userprintf_r.c
  ${QHULL_ROOT}/src/libqhull_r/io_r.c
  ${QHULL_ROOT}/src/libqhull_r/user_r.c
  ${QHULL_ROOT}/src/libqhull_r/rboxlib_r.c
  ${QHULL_ROOT}/src/libqhull_r/userprintf_rbox_r.c
)

set(_qhull_cpp_sources
  ${QHULL_ROOT}/src/libqhullcpp/Coordinates.cpp
  ${QHULL_ROOT}/src/libqhullcpp/PointCoordinates.cpp
  ${QHULL_ROOT}/src/libqhullcpp/Qhull.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullFacet.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullFacetList.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullFacetSet.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullHyperplane.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullPoint.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullPointSet.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullPoints.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullQh.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullRidge.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullSet.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullStat.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullUser.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullVertex.cpp
  ${QHULL_ROOT}/src/libqhullcpp/QhullVertexSet.cpp
  ${QHULL_ROOT}/src/libqhullcpp/RboxPoints.cpp
  ${QHULL_ROOT}/src/libqhullcpp/RoadError.cpp
  ${QHULL_ROOT}/src/libqhullcpp/RoadLogEvent.cpp
)

add_library(qhullstatic_r STATIC ${_qhull_r_sources})
target_include_directories(qhullstatic_r SYSTEM PUBLIC "${QHULL_ROOT}/src")
set_target_properties(qhullstatic_r PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  OUTPUT_NAME "qhullstatic_r$<$<CONFIG:Debug>:d>"
)

add_library(qhullcpp STATIC ${_qhull_cpp_sources})
target_include_directories(qhullcpp SYSTEM PUBLIC "${QHULL_ROOT}/src")
target_link_libraries(qhullcpp PUBLIC qhullstatic_r)
set_target_properties(qhullcpp PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  OUTPUT_NAME "qhullcpp$<$<CONFIG:Debug>:_d>"
)

if(UNIX)
  target_link_libraries(qhullstatic_r PUBLIC m)
endif()

if(MSVC)
  target_compile_definitions(qhullcpp PRIVATE qhullcpp_EXPORTS)
endif()

add_library(Qhull::qhullstatic_r ALIAS qhullstatic_r)
add_library(Qhull::qhullcpp ALIAS qhullcpp)
