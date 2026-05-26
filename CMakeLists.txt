cmake_minimum_required(VERSION 3.21)

project(
  tin_test
  VERSION 0.1.0
  DESCRIPTION "Synthetic TIN generator (C++)"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(_compile_commands_link "${CMAKE_SOURCE_DIR}/compile_commands.json")
  if(EXISTS "${_compile_commands_link}")
    file(REMOVE "${_compile_commands_link}")
  endif()
  file(CREATE_LINK
    "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${_compile_commands_link}"
    SYMBOLIC
  )
endif()

if(APPLE AND EXISTS "/opt/homebrew")
  list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

if(APPLE)
  if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Target macOS architectures")
  endif()
endif()

find_package(CGAL REQUIRED)
add_subdirectory(cmake/trimesh2)
include(cmake/find_qhull.cmake)

add_library(tin_gen
  src/app.cpp
  src/backends/generator_cgal.cpp
  src/backends/generator_trimesh2.cpp
  src/commands/generate.cpp
  src/convex_hull_3d.cpp
  src/convex_hull_vertices.cpp
  src/config.cpp
  src/cpu_timer.cpp
  src/mesh_format.cpp
  src/tin_mesh.cpp
  src/generator_save.cpp
)
target_include_directories(tin_gen PUBLIC include)
target_link_libraries(tin_gen
  PUBLIC
    CGAL::CGAL
    trimesh2
    Qhull::qhullcpp
    Qhull::qhullstatic_r
)

add_executable(tin_test src/main.cpp)
target_link_libraries(tin_test PRIVATE tin_gen)

if(MSVC)
  target_compile_options(tin_gen PRIVATE /W4 /permissive-)
  target_compile_options(tin_test PRIVATE /W4 /permissive-)
else()
  target_compile_options(tin_gen PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(tin_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_test(
  NAME tin_test_smoke
  COMMAND ${CMAKE_COMMAND}
    -DTIN_TEST=$<TARGET_FILE:tin_test>
    -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
    -P ${CMAKE_SOURCE_DIR}/cmake/run_smoke_test.cmake
)
