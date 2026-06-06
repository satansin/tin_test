#include "tin_gen/config.hpp"

#include "tin_gen/distance_algorithm.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace tin_gen {
namespace {

std::string need_arg_value(const int argc, char* argv[], int& index, const char* name) {
  if (index + 1 >= argc) {
    throw std::runtime_error(std::string("Missing value for ") + name);
  }
  return argv[++index];
}

bool is_help_flag(const std::string_view arg) {
  return arg == "-h" || arg == "--help";
}

bool accepts_positional_input_dir(const std::string_view arg, const std::string& input_dir) {
  return !arg.empty() && arg[0] != '-' && input_dir.empty();
}

void require_input_dir(const std::string& input_dir, const std::string_view command) {
  if (input_dir.empty()) {
    throw std::runtime_error(std::string(command) +
                             " requires --input-dir DIR (or a positional DIR)");
  }
}

}  // namespace

std::optional<GenerationConfig> parse_generate_config(const int argc, char* argv[]) {
  GenerationConfig config;

  for (int i = 0; i < argc; ++i) {
    const std::string arg = argv[i];

    if (is_help_flag(arg)) {
      return std::nullopt;
    }
    if (arg == "--format") {
      config.format = parse_mesh_format(need_arg_value(argc, argv, i, "--format"));
    } else if (arg == "-o" || arg == "--output-dir") {
      config.output_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--num-objects") {
      config.num_objects = static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, "--num-objects")));
    } else if (arg == "--num-vertices-per-object") {
      config.num_vertices_per_object =
          static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, "--num-vertices-per-object")));
    } else if (arg == "--scale") {
      config.scale = std::stod(need_arg_value(argc, argv, i, "--scale"));
    } else if (arg == "--seed") {
      config.random_seed = static_cast<unsigned>(std::stoul(need_arg_value(argc, argv, i, "--seed")));
    } else if (arg == "--quiet" || arg == "-q") {
      config.quiet = true;
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  if (config.num_objects == 0) {
    throw std::runtime_error("num_objects must be > 0");
  }
  if (config.num_vertices_per_object < 4) {
    throw std::runtime_error("num_vertices_per_object must be >= 4 for a 3D convex hull");
  }
  if (config.scale <= 0.0) {
    throw std::runtime_error("scale must be > 0");
  }

  return config;
}

std::optional<NormalizeConfig> parse_normalize_config(const int argc, char* argv[]) {
  NormalizeConfig config;

  for (int i = 0; i < argc; ++i) {
    const std::string arg = argv[i];

    if (is_help_flag(arg)) {
      return std::nullopt;
    }
    if (arg == "--input-dir" || arg == "-i") {
      config.input_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--output-dir" || arg == "-o") {
      config.output_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--max-objects" || arg == "--limit") {
      config.max_objects = static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (accepts_positional_input_dir(arg, config.input_dir)) {
      config.input_dir = arg;
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  require_input_dir(config.input_dir, "normalize");
  return config;
}

IndexVerticesConfig default_index_vertices_config(const VertexIndexKind kind) {
  IndexVerticesConfig config;
  config.kind = kind;
  if (kind == VertexIndexKind::Kd) {
    config.output_dir = "sample_kdvertices";
  } else {
    config.output_dir = "sample_rsvertices";
  }
  return config;
}

std::optional<IndexVerticesConfig> parse_index_vertices_config(const int argc, char* argv[],
                                                               const VertexIndexKind kind) {
  IndexVerticesConfig config = default_index_vertices_config(kind);
  const char* command_name = kind == VertexIndexKind::Kd ? "kd" : "rs";

  for (int i = 0; i < argc; ++i) {
    const std::string arg = argv[i];

    if (is_help_flag(arg)) {
      return std::nullopt;
    }
    if (arg == "--input-dir" || arg == "-i") {
      config.input_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--output-dir" || arg == "-o") {
      config.output_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--max-objects" || arg == "--limit") {
      config.max_objects = static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (arg == "--combined" || arg == "--bundle") {
      config.combined_output = true;
    } else if (accepts_positional_input_dir(arg, config.input_dir)) {
      config.input_dir = arg;
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  require_input_dir(config.input_dir, command_name);
  return config;
}

std::optional<CompressConfig> parse_compress_config(const int argc, char* argv[]) {
  CompressConfig config;

  for (int i = 0; i < argc; ++i) {
    const std::string arg = argv[i];

    if (is_help_flag(arg)) {
      return std::nullopt;
    }
    if (arg == "--input-dir" || arg == "-i") {
      config.input_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--output-dir" || arg == "-o") {
      config.output_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--max-meshes-per-bundle") {
      config.max_meshes_per_bundle =
          static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (arg == "--max-objects" || arg == "--limit") {
      config.max_objects = static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (accepts_positional_input_dir(arg, config.input_dir)) {
      config.input_dir = arg;
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  require_input_dir(config.input_dir, "compress");
  return config;
}

std::optional<DistanceConfig> parse_distance_config(const int argc, char* argv[]) {
  DistanceConfig config;

  for (int i = 0; i < argc; ++i) {
    const std::string arg = argv[i];

    if (is_help_flag(arg)) {
      return std::nullopt;
    }
    if (arg == "--algorithm") {
      config.algorithm = parse_distance_algorithm(need_arg_value(argc, argv, i, "--algorithm"));
    } else if (arg == "--compare-index" || arg == "--compare-spatial-index") {
      config.compare_spatial_index = true;
    } else if (arg == "--a" || arg == "--mesh-a") {
      config.path_a = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--b" || arg == "--mesh-b") {
      config.path_b = need_arg_value(argc, argv, i, arg.c_str());
    } else if (!arg.empty() && arg[0] != '-') {
      if (config.path_a.empty()) {
        config.path_a = arg;
      } else if (config.path_b.empty()) {
        config.path_b = arg;
      } else {
        throw std::runtime_error("Unexpected positional argument: " + arg);
      }
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  if (config.path_a.empty() || config.path_b.empty()) {
    throw std::runtime_error("distance requires two PLY paths: distance A.ply B.ply");
  }

  return config;
}

std::string default_pairwise_distance_output_path(const DistanceAlgorithm algorithm) {
  return std::string("sample_pd/pairwise_distances_") +
         std::string(distance_algorithm_name(algorithm)) + ".txt";
}

std::optional<PairwiseDistanceConfig> parse_pairwise_distance_config(const int argc,
                                                                     char* argv[]) {
  PairwiseDistanceConfig config;
  bool output_set = false;

  for (int i = 0; i < argc; ++i) {
    const std::string arg = argv[i];

    if (is_help_flag(arg)) {
      return std::nullopt;
    }
    if (arg == "--input-dir" || arg == "-i") {
      config.input_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--output" || arg == "-o") {
      config.output_path = need_arg_value(argc, argv, i, arg.c_str());
      output_set = true;
    } else if (arg == "--algorithm") {
      config.algorithm = parse_distance_algorithm(need_arg_value(argc, argv, i, "--algorithm"));
    } else if (arg == "--max-objects" || arg == "--limit") {
      config.max_objects = static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (arg == "--rs-dir" || arg == "-rs") {
      config.rs_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--kd-dir" || arg == "-kd") {
      config.kd_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (accepts_positional_input_dir(arg, config.input_dir)) {
      config.input_dir = arg;
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  require_input_dir(config.input_dir, "pairwise_distance");

  if (!output_set) {
    config.output_path = default_pairwise_distance_output_path(config.algorithm);
  }

  return config;
}

}  // namespace tin_gen
