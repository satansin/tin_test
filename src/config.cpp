#include "tin_gen/config.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace tin_gen {
namespace {

std::string lower(std::string_view value) {
  std::string out(value);
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

}  // namespace

GeneratorBackend parse_generator_backend(const std::string_view value) {
  const std::string normalized = lower(value);
  if (normalized == "cgal") {
    return GeneratorBackend::Cgal;
  }
  if (normalized == "trimesh" || normalized == "trimesh2") {
    return GeneratorBackend::Trimesh2;
  }
  throw std::invalid_argument("Unsupported backend: " + std::string(value) +
                              " (expected cgal or trimesh2)");
}

std::string_view generator_backend_name(const GeneratorBackend backend) {
  switch (backend) {
    case GeneratorBackend::Cgal:
      return "cgal";
    case GeneratorBackend::Trimesh2:
      return "trimesh2";
  }
  return "cgal";
}

std::optional<AppConfig> parse_generate_config(const int argc, char* argv[]) {
  AppConfig config;

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
    if (arg == "--backend") {
      config.backend = parse_generator_backend(need_value("--backend"));
    } else if (arg == "--format") {
      config.format = parse_mesh_format(need_value("--format"));
    } else if (arg == "-o" || arg == "--output-dir") {
      config.output_dir = need_value(arg.c_str());
    } else if (arg == "--num-objects") {
      config.num_objects = static_cast<std::size_t>(std::stoull(need_value("--num-objects")));
    } else if (arg == "--num-vertices-per-object") {
      config.num_vertices_per_object =
          static_cast<std::size_t>(std::stoull(need_value("--num-vertices-per-object")));
    } else if (arg == "--scale") {
      config.scale = std::stod(need_value("--scale"));
    } else if (arg == "--seed") {
      config.random_seed = static_cast<unsigned>(std::stoul(need_value("--seed")));
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

}  // namespace tin_gen
