#pragma once

#include "tin_gen/distance_algorithm.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/ply_merge.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace tin_gen {

struct GenerationConfig {
  MeshFormat format = MeshFormat::Ply;
  std::size_t num_objects = 10;
  std::size_t num_vertices_per_object = 200;
  double scale = 1.0;
  std::string output_dir;
  unsigned random_seed = 0;
  bool quiet = false;
};

struct NormalizeConfig {
  std::string input_dir;
  std::string output_dir;
  std::size_t max_objects = 0;  // 0 = all
};

/// KD-tree or R*-tree vertex index build (`kd` / `rs` commands).
enum class VertexIndexKind { Kd, Rs };

struct IndexVerticesConfig {
  VertexIndexKind kind = VertexIndexKind::Rs;
  std::string input_dir;
  std::string output_dir;
  std::size_t max_objects = 0;  // 0 = all
  bool combined_output = false;
};

[[nodiscard]] IndexVerticesConfig default_index_vertices_config(VertexIndexKind kind);

/// Parse options for the kd / rs index commands. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<IndexVerticesConfig> parse_index_vertices_config(int argc,
                                                                               char* argv[],
                                                                               VertexIndexKind kind);

/// Parse options for the generate command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<GenerationConfig> parse_generate_config(int argc, char* argv[]);

/// Parse options for the normalize command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<NormalizeConfig> parse_normalize_config(int argc, char* argv[]);

struct CompressConfig {
  std::string input_dir;
  std::string output_dir;
  std::size_t max_meshes_per_bundle = kDefaultMaxMeshesPerPlyBundle;
  std::size_t max_objects = 0;  // 0 = all
};

/// Parse options for the compress command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<CompressConfig> parse_compress_config(int argc, char* argv[]);

struct DistanceConfig {
  std::string path_a;
  std::string path_b;
  DistanceAlgorithm algorithm = DistanceAlgorithm::Vertex;
  /// When true, build KD-tree and R*-tree and print a side-by-side timing comparison.
  bool compare_spatial_index = false;
};

/// Parse options for the distance command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<DistanceConfig> parse_distance_config(int argc, char* argv[]);

struct PointSampleConfig {
  std::string input_dir;
  std::string output_dir;
  MeshFormat format = MeshFormat::Ply;
  std::size_t num_points = 0;
  std::size_t max_objects = 0;  // 0 = all
  std::size_t max_meshes_per_bundle = kDefaultMaxMeshesPerPlyBundle;
  bool pack_output = false;
  unsigned random_seed = 0;
};

/// Parse options for the ptsample command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<PointSampleConfig> parse_point_sample_config(int argc,
                                                                           char* argv[]);

struct PairwiseDistanceConfig {
  std::string input_dir;
  std::string output_path;
  DistanceAlgorithm algorithm = DistanceAlgorithm::Vertex;
  std::size_t max_objects = 0;  // 0 = all
  /// When set (`--rs-dir`), load per-mesh `<stem>.rstree` files from this folder for NN search.
  std::optional<std::string> rs_dir;
  /// When set (`--kd-dir`), load per-mesh `<stem>.kdtree` files instead of R*-trees.
  std::optional<std::string> kd_dir;
};

/// Parse options for the pairwise_distance command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<PairwiseDistanceConfig> parse_pairwise_distance_config(int argc,
                                                                                   char* argv[]);

struct ValidateConfig {
  std::string input_dir;
  std::string report_path;   // optional; empty = no report file
  std::size_t max_objects = 0;  // 0 = all
};

/// Parse options for the validate command. Returns nullopt on -h/--help.
[[nodiscard]] std::optional<ValidateConfig> parse_validate_config(int argc, char* argv[]);

}  // namespace tin_gen
