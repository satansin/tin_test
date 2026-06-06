#include "tin_gen/commands/generate.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/generator.hpp"

#include <cstdlib>
#include <iostream>

namespace tin_gen {

int run_generate(const GenerationConfig& config) {
  CpuTimer cpu_timer;
  WallTimer wall_timer;
  cpu_timer.start();
  wall_timer.start();
  generate_and_save_objects(config);
  cpu_timer.stop();
  wall_timer.stop();
  std::cout << "generate CPU time: ";
  append_formatted_elapsed_seconds(std::cout, cpu_timer.elapsed_seconds());
  std::cout << "\ngenerate wall time: ";
  append_formatted_elapsed_seconds(std::cout, wall_timer.elapsed_seconds());
  std::cout << '\n';

  if (!config.quiet) {
    std::cout << "Generated " << config.num_objects << " TIN(s) as "
              << mesh_format_name(config.format) << " (" << config.num_vertices_per_object
              << " hull vertices each).\n";
  }

  return EXIT_SUCCESS;
}

}  // namespace tin_gen
