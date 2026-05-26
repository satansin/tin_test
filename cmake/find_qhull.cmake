# Always build Qhull from source under third_party/qhull (never use a system install).
include(FetchContent)

set(TIN_QHULL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/qhull")

FetchContent_Declare(
  qhull
  GIT_REPOSITORY https://github.com/qhull/qhull.git
  GIT_TAG        v8.0.2
  SOURCE_DIR     ${TIN_QHULL_SOURCE_DIR}
  UPDATE_DISCONNECTED TRUE
)

message(STATUS "Fetching/building Qhull in ${TIN_QHULL_SOURCE_DIR}")
FetchContent_MakeAvailable(qhull)

if(NOT TARGET qhullstatic_r)
  message(FATAL_ERROR "Qhull fetch succeeded but target 'qhullstatic_r' was not created.")
endif()
if(NOT TARGET qhullcpp)
  message(FATAL_ERROR "Qhull fetch succeeded but target 'qhullcpp' was not created.")
endif()

if(NOT TARGET Qhull::qhullstatic_r)
  add_library(Qhull::qhullstatic_r ALIAS qhullstatic_r)
endif()
if(NOT TARGET Qhull::qhullcpp)
  add_library(Qhull::qhullcpp ALIAS qhullcpp)
endif()

# Headers are included as <libqhullcpp/...> relative to src/.
set(TIN_QHULL_INCLUDE_DIR "${TIN_QHULL_SOURCE_DIR}/src" CACHE INTERNAL "")
