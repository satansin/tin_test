# Runs tin_test generate in an isolated directory and checks PLY output.
if(NOT TIN_TEST)
  message(FATAL_ERROR "TIN_TEST not set")
endif()

set(work_dir "${CMAKE_BINARY_DIR}/test_smoke")
set(output_dir "${work_dir}/output")

file(REMOVE_RECURSE "${work_dir}")
file(MAKE_DIRECTORY "${work_dir}")

execute_process(
  COMMAND "${TIN_TEST}" generate --num-objects 5 --num-vertices-per-object 20 --seed 1
  WORKING_DIRECTORY "${work_dir}"
  RESULT_VARIABLE run_result
  OUTPUT_VARIABLE run_output
  ERROR_VARIABLE run_error
)
if(run_result)
  message(FATAL_ERROR "tin_test failed (${run_result}):\n${run_output}\n${run_error}")
endif()

foreach(i RANGE 1 5)
  set(mesh_file "${output_dir}/object_${i}.ply")
  if(NOT EXISTS "${mesh_file}")
    message(FATAL_ERROR "Missing expected file: ${mesh_file}")
  endif()
  file(SIZE "${mesh_file}" mesh_size)
  if(mesh_size LESS 100)
    message(FATAL_ERROR "PLY file too small: ${mesh_file}")
  endif()
endforeach()

message(STATUS "Smoke test passed: 5 PLY files in ${output_dir}")
