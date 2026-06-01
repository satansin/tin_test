#include "tin_gen/config.hpp"

#include <stdexcept>
#include <string>

namespace tin_gen {

std::optional<KdVerticesConfig> parse_kdvertices_config(const int argc, char* argv[]) {
  KdVerticesConfig config;

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
    } else if (arg == "--output-dir" || arg == "-o") {
      config.output_dir = need_value(arg.c_str());
    } else if (arg == "--max-objects" || arg == "--limit") {
      config.max_objects = static_cast<std::size_t>(std::stoull(need_value(arg.c_str())));
    } else if (arg == "--combined" || arg == "--bundle") {
      config.combined_output = true;
    } else if (arg == "--combined-file") {
      config.combined_output = true;
      config.combined_file = need_value("--combined-file");
    } else if (!arg.empty() && arg[0] != '-' && config.input_dir.empty()) {
      config.input_dir = arg;
    } else {
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  if (config.input_dir.empty()) {
    throw std::runtime_error("kdvertices requires --input-dir DIR (or a positional DIR)");
  }

  return config;
}

}  // namespace tin_gen
