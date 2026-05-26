#include "tin_gen/commands/generate.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/generator.hpp"

#include <cstdlib>
#include <iostream>

namespace tin_gen {

int run_generate(const AppConfig& config) {
  CpuTimer cpu_timer;
  WallTimer wall_timer;
  cpu_timer.start();
  wall_timer.start();
  const auto objects =
      generate_random_tin(config.num_objects, config.num_vertices_per_object, config.scale,
                          config.random_seed);
  cpu_timer.stop();
  wall_timer.stop();
  std::cout << "generate CPU time: " << cpu_timer.elapsed_seconds() << " s\n";
  std::cout << "generate wall time: " << wall_timer.elapsed_seconds() << " s\n";

  save_objects_as_files(objects, config.output_dir, config.format);

  if (!objects.empty()) {
    std::cout << "Generated " << objects.size() << " TIN(s) as "
              << mesh_format_name(config.format) << ". First mesh: "
              << objects.front().vertices.size() << " vertices, "
              << objects.front().faces.size() << " triangles.\n";
  }

  return EXIT_SUCCESS;
}

}  // namespace tin_gen
