#pragma once

#include "tin_gen/mesh_format.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace tin_gen {

struct AppConfig {
  MeshFormat format = MeshFormat::Ply;
  std::size_t num_objects = 10;
  std::size_t num_vertices_per_object = 200;
  double scale = 1.0;
  std::string output_dir = "output";
  unsigned random_seed = 0;
};

/// Parse options for the generate command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<AppConfig> parse_generate_config(int argc, char* argv[]);

}  // namespace tin_gen
