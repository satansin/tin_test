# Fetch Qhull sources into third_party/qhull, then build only qhullstatic_r + qhullcpp.
include(FetchContent)

set(TIN_QHULL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/qhull")

FetchContent_Declare(
  qhull
  GIT_REPOSITORY https://github.com/qhull/qhull.git
  GIT_TAG        v8.0.2
  SOURCE_DIR     ${TIN_QHULL_SOURCE_DIR}
  UPDATE_DISCONNECTED TRUE
)

message(STATUS "Fetching Qhull sources into ${TIN_QHULL_SOURCE_DIR}")
FetchContent_GetProperties(qhull)
if(NOT qhull_POPULATED)
  FetchContent_Populate(qhull)
endif()

if(NOT EXISTS "${TIN_QHULL_SOURCE_DIR}/src/libqhullcpp/Qhull.h")
  message(FATAL_ERROR "Qhull sources missing at ${TIN_QHULL_SOURCE_DIR}")
endif()

enable_language(C)

set(QHULL_ROOT "${TIN_QHULL_SOURCE_DIR}")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/qhull_vendor" "${CMAKE_BINARY_DIR}/qhull-vendor")

if(NOT TARGET qhullstatic_r OR NOT TARGET qhullcpp)
  message(FATAL_ERROR "Qhull vendor libraries were not created.")
endif()

set(TIN_QHULL_INCLUDE_DIR "${TIN_QHULL_SOURCE_DIR}/src" CACHE INTERNAL "")
