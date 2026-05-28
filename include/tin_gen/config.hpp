#pragma once

#include "tin_gen/mesh_format.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace tin_gen {

struct GenerationConfig {
  MeshFormat format = MeshFormat::Ply;
  std::size_t num_objects = 10;
  std::size_t num_vertices_per_object = 200;
  double scale = 1.0;
  std::string output_dir = "sample_gen";
  unsigned random_seed = 0;
  bool quiet = false;
};

struct NormalizeConfig {
  std::string input_dir;
  std::string output_dir = "sample_normalized";
  /// When true, use metadata_path if set, else <input-dir>/metadata.txt when present.
  bool use_metadata = true;
  std::optional<std::string> metadata_path;
  std::size_t max_objects = 0;  // 0 = all
};

/// Parse options for the generate command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<GenerationConfig> parse_generate_config(int argc, char* argv[]);

/// Parse options for the normalize command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<NormalizeConfig> parse_normalize_config(int argc, char* argv[]);

}  // namespace tin_gen
