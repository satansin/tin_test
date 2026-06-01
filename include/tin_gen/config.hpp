#pragma once

#include "tin_gen/distance_algorithm.hpp"
#include "tin_gen/mesh_helper.hpp"

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

struct KdVerticesConfig {
  std::string input_dir;
  std::string output_dir = "sample_kdvertices";
  std::size_t max_objects = 0;  // 0 = all
  /// Write one combined bundle file instead of per-mesh .kdtree files.
  bool combined_output = false;
  std::string combined_file = "combined.kdtree";
};

/// Parse options for the generate command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<GenerationConfig> parse_generate_config(int argc, char* argv[]);

/// Parse options for the normalize command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<NormalizeConfig> parse_normalize_config(int argc, char* argv[]);

/// Parse options for the kdvertices command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<KdVerticesConfig> parse_kdvertices_config(int argc, char* argv[]);

struct DistanceConfig {
  std::string path_a;
  std::string path_b;
  DistanceAlgorithm algorithm = DistanceAlgorithm::Vertex;
};

/// Parse options for the distance command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<DistanceConfig> parse_distance_config(int argc, char* argv[]);

struct PairwiseDistanceConfig {
  std::string input_dir;
  std::string output_path;  // default: sample_pd/pairwise_distances_<algorithm>.txt
  DistanceAlgorithm algorithm = DistanceAlgorithm::Vertex;
  std::size_t max_objects = 0;  // 0 = all
  /// When set (`--kd-dir`), load per-mesh `<stem>.kdtree` files from this folder for NN search.
  std::optional<std::string> kdtree_dir;
};

[[nodiscard]] std::string default_pairwise_distance_output_path(DistanceAlgorithm algorithm);

/// Parse options for the pairwise_distance command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<PairwiseDistanceConfig> parse_pairwise_distance_config(int argc,
                                                                                   char* argv[]);

}  // namespace tin_gen
