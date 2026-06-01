#include "tin_gen/config.hpp"

#include "tin_gen/distance_algorithm.hpp"

#include <stdexcept>
#include <string>

namespace tin_gen {

std::optional<DistanceConfig> parse_distance_config(const int argc, char* argv[]) {
  DistanceConfig config;

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
    if (arg == "--algorithm") {
      config.algorithm = parse_distance_algorithm(need_value("--algorithm"));
    } else if (arg == "--a" || arg == "--mesh-a") {
      config.path_a = need_value(arg.c_str());
    } else if (arg == "--b" || arg == "--mesh-b") {
      config.path_b = need_value(arg.c_str());
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

}  // namespace tin_gen
