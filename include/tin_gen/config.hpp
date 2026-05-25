#pragma once

#include "tin_gen/mesh_format.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace tin_gen {

enum class GeneratorBackend { Cgal, Trimesh2 };

struct AppConfig {
  GeneratorBackend backend = GeneratorBackend::Cgal;
  MeshFormat format = MeshFormat::Ply;
  std::size_t num_objects = 10;
  std::size_t num_vertices_per_object = 200;
  double scale = 1.0;
  std::string output_dir = "output";
  unsigned random_seed = 0;
};

[[nodiscard]] GeneratorBackend parse_generator_backend(std::string_view value);
[[nodiscard]] std::string_view generator_backend_name(GeneratorBackend backend);

/// Parse options for the generate command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<AppConfig> parse_generate_config(int argc, char* argv[]);

}  // namespace tin_gen
