#include "tin_gen/config.hpp"

#include "tin_gen/distance_algorithm.hpp"

#include <stdexcept>
#include <string>

namespace tin_gen {

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
    auto need_value = [&](const char* name) -> std::string {
      if (i + 1 >= argc) {
        throw std::runtime_error(std::string("Missing value for ") + name);
      }
      return argv[++i];
    };

    if (arg == "-h" || arg == "--help") {
      return std::nullopt;
    }
    if (arg == "--input-dir" || arg == "-i") {
      config.input_dir = need_value(arg.c_str());
    } else if (arg == "--output" || arg == "-o") {
      config.output_path = need_value(arg.c_str());
      output_set = true;
    } else if (arg == "--algorithm") {
      config.algorithm = parse_distance_algorithm(need_value("--algorithm"));
    } else if (arg == "--max-objects" || arg == "--limit") {
      config.max_objects = static_cast<std::size_t>(std::stoull(need_value(arg.c_str())));
    } else if (arg == "--kd-dir" || arg == "--kdtree-dir" || arg == "--kdvertices-dir") {
      config.kdtree_dir = need_value(arg.c_str());
    } else if (!arg.empty() && arg[0] != '-' && config.input_dir.empty()) {
      config.input_dir = arg;
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  if (config.input_dir.empty()) {
    throw std::runtime_error(
        "pairwise_distance requires --input-dir DIR (or a positional DIR)");
  }

  if (!output_set) {
    config.output_path = default_pairwise_distance_output_path(config.algorithm);
  }

  return config;
}

}  // namespace tin_gen
