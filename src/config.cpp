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

void require_output_dir(const std::string& output_dir, const std::string_view command) {
  if (output_dir.empty()) {
    throw std::runtime_error(std::string(command) + " requires --output-dir DIR");
  }
}

void require_output_path(const std::string& output_path, const std::string_view command) {
  if (output_path.empty()) {
    throw std::runtime_error(std::string(command) + " requires --output PATH");
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
  require_output_dir(config.output_dir, "generate");

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
  require_output_dir(config.output_dir, "normalize");
  return config;
}

IndexVerticesConfig default_index_vertices_config(const VertexIndexKind kind) {
  IndexVerticesConfig config;
  config.kind = kind;
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
  require_output_dir(config.output_dir, command_name);
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
  require_output_dir(config.output_dir, "compress");
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

std::optional<PointSampleConfig> parse_point_sample_config(const int argc, char* argv[]) {
  PointSampleConfig config;

  for (int i = 0; i < argc; ++i) {
    const std::string arg = argv[i];

    if (is_help_flag(arg)) {
      return std::nullopt;
    }
    if (arg == "--input-dir" || arg == "-i") {
      config.input_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--output-dir" || arg == "-o") {
      config.output_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--format") {
      config.format = parse_mesh_format(need_arg_value(argc, argv, i, "--format"));
    } else if (arg == "--num-points" || arg == "--points" || arg == "-n") {
      config.num_points =
          static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (arg == "--max-objects" || arg == "--limit") {
      config.max_objects =
          static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (arg == "--max-meshes-per-bundle") {
      config.max_meshes_per_bundle =
          static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (arg == "--pack") {
      config.pack_output = true;
    } else if (arg == "--seed") {
      config.random_seed =
          static_cast<unsigned>(std::stoul(need_arg_value(argc, argv, i, "--seed")));
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  require_input_dir(config.input_dir, "ptsample");
  require_output_dir(config.output_dir, "ptsample");
  if (config.num_points == 0) {
    throw std::runtime_error("ptsample requires --num-points N with N > 0");
  }
  if (config.max_meshes_per_bundle == 0) {
    throw std::runtime_error("ptsample: max_meshes_per_bundle must be > 0");
  }
  if (config.pack_output && config.format != MeshFormat::Ply) {
    throw std::runtime_error("ptsample: --pack requires --format ply");
  }

  return config;
}

std::optional<PairwiseDistanceConfig> parse_pairwise_distance_config(const int argc,
                                                                     char* argv[]) {
  PairwiseDistanceConfig config;

  for (int i = 0; i < argc; ++i) {
    const std::string arg = argv[i];

    if (is_help_flag(arg)) {
      return std::nullopt;
    }
    if (arg == "--input-dir" || arg == "-i") {
      config.input_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--output" || arg == "-o") {
      config.output_path = need_arg_value(argc, argv, i, arg.c_str());
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
  require_output_path(config.output_path, "pairwise_distance");

  return config;
}

std::optional<TopKConfig> parse_topk_config(const int argc, char* argv[]) {
  TopKConfig config;

  for (int i = 0; i < argc; ++i) {
    const std::string arg = argv[i];

    if (is_help_flag(arg)) {
      return std::nullopt;
    }
    if (arg == "--input-dir" || arg == "-i") {
      config.input_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--query" || arg == "-q") {
      config.query_path = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--k" || arg == "-k") {
      config.k = static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (arg == "--algorithm") {
      config.algorithm = parse_distance_algorithm(need_arg_value(argc, argv, i, "--algorithm"));
    } else if (arg == "--max-objects" || arg == "--limit") {
      config.max_objects =
          static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (arg == "--rs-dir" || arg == "-rs") {
      config.rs_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--kd-dir" || arg == "-kd") {
      config.kd_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--output" || arg == "-o") {
      config.output_path = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--include-self") {
      config.include_self = true;
    } else if (accepts_positional_input_dir(arg, config.input_dir)) {
      config.input_dir = arg;
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  require_input_dir(config.input_dir, "topk");
  if (config.query_path.empty()) {
    throw std::runtime_error("topk requires --query PATH");
  }
  if (config.k == 0) {
    throw std::runtime_error("topk requires --k N with N > 0");
  }

  return config;
}

std::optional<ValidateConfig> parse_validate_config(const int argc, char* argv[]) {
  ValidateConfig config;

  for (int i = 0; i < argc; ++i) {
    const std::string arg = argv[i];

    if (is_help_flag(arg)) {
      return std::nullopt;
    }
    if (arg == "--input-dir" || arg == "-i") {
      config.input_dir = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--report" || arg == "--output" || arg == "-o") {
      config.report_path = need_arg_value(argc, argv, i, arg.c_str());
    } else if (arg == "--max-objects" || arg == "--limit") {
      config.max_objects =
          static_cast<std::size_t>(std::stoull(need_arg_value(argc, argv, i, arg.c_str())));
    } else if (accepts_positional_input_dir(arg, config.input_dir)) {
      config.input_dir = arg;
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  require_input_dir(config.input_dir, "validate");
  return config;
}

}  // namespace tin_gen
