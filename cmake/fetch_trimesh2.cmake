# Always fetch TriMesh2 into third_party/trimesh2 (no manual clone).
include(FetchContent)

set(TIN_TRIMESH2_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/trimesh2")

FetchContent_Declare(
  trimesh2_src
  GIT_REPOSITORY https://github.com/Forceflow/trimesh2.git
  GIT_TAG        2022.03.04
  SOURCE_DIR     ${TIN_TRIMESH2_SOURCE_DIR}
  UPDATE_DISCONNECTED TRUE
)

message(STATUS "Fetching TriMesh2 into ${TIN_TRIMESH2_SOURCE_DIR}")
FetchContent_MakeAvailable(trimesh2_src)

if(NOT EXISTS "${TIN_TRIMESH2_SOURCE_DIR}/include/TriMesh.h")
  message(FATAL_ERROR "TriMesh2 fetch failed: ${TIN_TRIMESH2_SOURCE_DIR}/include/TriMesh.h not found.")
endif()

set(TRIMESH2_ROOT "${TIN_TRIMESH2_SOURCE_DIR}" CACHE INTERNAL "")
