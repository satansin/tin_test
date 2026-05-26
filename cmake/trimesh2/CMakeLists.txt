# Static TriMesh2 library (IO + mesh ops, no OpenGL utilities).
# TRIMESH2_ROOT is set by cmake/fetch_trimesh2.cmake before add_subdirectory().
if(NOT TRIMESH2_ROOT)
  set(TRIMESH2_ROOT "${CMAKE_SOURCE_DIR}/third_party/trimesh2")
endif()

set(TRIMESH2_SOURCES
  ${TRIMESH2_ROOT}/libsrc/TriMesh_bounding.cc
  ${TRIMESH2_ROOT}/libsrc/TriMesh_connectivity.cc
  ${TRIMESH2_ROOT}/libsrc/TriMesh_curvature.cc
  ${TRIMESH2_ROOT}/libsrc/TriMesh_io.cc
  ${TRIMESH2_ROOT}/libsrc/TriMesh_grid.cc
  ${TRIMESH2_ROOT}/libsrc/TriMesh_normals.cc
  ${TRIMESH2_ROOT}/libsrc/TriMesh_pointareas.cc
  ${TRIMESH2_ROOT}/libsrc/TriMesh_stats.cc
  ${TRIMESH2_ROOT}/libsrc/TriMesh_tstrips.cc
  ${TRIMESH2_ROOT}/libsrc/ICP.cc
  ${TRIMESH2_ROOT}/libsrc/KDtree.cc
  ${TRIMESH2_ROOT}/libsrc/conn_comps.cc
  ${TRIMESH2_ROOT}/libsrc/diffuse.cc
  ${TRIMESH2_ROOT}/libsrc/edgeflip.cc
  ${TRIMESH2_ROOT}/libsrc/faceflip.cc
  ${TRIMESH2_ROOT}/libsrc/filter.cc
  ${TRIMESH2_ROOT}/libsrc/make.cc
  ${TRIMESH2_ROOT}/libsrc/merge.cc
  ${TRIMESH2_ROOT}/libsrc/overlap.cc
  ${TRIMESH2_ROOT}/libsrc/remove.cc
  ${TRIMESH2_ROOT}/libsrc/reorder_verts.cc
  ${TRIMESH2_ROOT}/libsrc/subdiv.cc
  ${TRIMESH2_ROOT}/libsrc/umbrella.cc
)

add_library(trimesh2 STATIC ${TRIMESH2_SOURCES})
target_include_directories(trimesh2 SYSTEM PUBLIC "${TRIMESH2_ROOT}/include")
target_compile_features(trimesh2 PUBLIC cxx_std_14)

if(UNIX AND NOT APPLE)
  target_link_libraries(trimesh2 PUBLIC m)
endif()

target_compile_options(trimesh2 PRIVATE
  $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wno-unused-parameter -Wno-sign-compare>
)
