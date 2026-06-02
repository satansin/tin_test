#include "tin_gen/app.hpp"

#include "tin_gen/commands/generate.hpp"
#include "tin_gen/commands/distance.hpp"
#include "tin_gen/commands/kdvertices.hpp"
#include "tin_gen/commands/pairwise_distance.hpp"
#include "tin_gen/commands/normalize.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace tin_gen {
namespace {

std::string lower(std::string_view value) {
  std::string out(value);
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

bool is_option(const char* arg) {
  return arg != nullptr && arg[0] == '-';
}

}  // namespace

AppCommand parse_app_command(const std::string_view name) {
  const std::string normalized = lower(name);
  if (normalized == "generate" || normalized == "gen") {
    return AppCommand::Generate;
  }
  if (normalized == "normalize" || normalized == "norm") {
    return AppCommand::Normalize;
  }
  if (normalized == "kdvertices" || normalized == "kd") {
    return AppCommand::KdVertices;
  }
  if (normalized == "distance" || normalized == "dist") {
    return AppCommand::Distance;
  }
  if (normalized == "pairwise_distance" || normalized == "pairwisedistance" ||
      normalized == "pd") {
    return AppCommand::PairwiseDistance;
  }
  throw std::invalid_argument("Unknown command: " + std::string(name));
}

std::string_view app_command_name(const AppCommand command) {
  switch (command) {
    case AppCommand::Generate:
      return "generate";
    case AppCommand::Normalize:
      return "normalize";
    case AppCommand::KdVertices:
      return "kdvertices";
    case AppCommand::Distance:
      return "distance";
    case AppCommand::PairwiseDistance:
      return "pairwise_distance";
  }
  return "generate";
}

void print_usage(const char* program) {
  std::cout << "Usage: " << program << " [command] [options]\n\n"
            << "Commands:\n"
            << "  generate (gen)   Build random TIN meshes and write files\n"
            << "  normalize (norm) Translate PLY meshes to zero mean (no scaling)\n"
            << "  kdvertices (kd)  Build KD-tree vertex indexes for meshes in a folder\n"
            << "  distance (dist)    Dissimilarity between two PLY meshes\n"
            << "  pairwise_distance (pd)  All pairwise dissimilarities in a folder\n"
            << "  help                    Show this help\n\n"
            << "Command aliases: gen, norm, kd, dist, pd\n\n"
            << "Generate options:\n"
            << "  --format FORMAT     Output mesh format: ply | obj (default: ply)\n"
            << "  -o, --output-dir DIR   Output directory (default: sample_gen)\n"
            << "  --num-objects N     Number of meshes to generate (default: 10)\n"
            << "  --num-vertices-per-object N   Vertices on each convex hull (default: 200)\n"
            << "  --scale VALUE       Random point coordinate scale (default: 1)\n"
            << "  --seed N            RNG seed, 0 = random (default: 0)\n"
            << "  -q, --quiet         Progress every 1000 meshes (not every file)\n\n"
            << "Normalize options:\n"
            << "  -i, --input-dir DIR    Folder containing .ply files\n"
            << "  -o, --output-dir DIR   Output directory (default: sample_normalized)\n"
            << "  --max-objects N        Normalize at most N meshes (default: all)\n\n"
            << "Kdvertices options:\n"
            << "  -i, --input-dir DIR    Folder containing .ply files\n"
            << "  -o, --output-dir DIR   Output directory (default: sample_kdvertices)\n"
            << "  --max-objects N        Process at most N meshes (default: all)\n"
            << "  --combined             Write one bundle file (default: combined.kdtree)\n"
            << "  --combined-file PATH   Bundle filename with --combined\n\n"
            << "Distance options:\n"
            << "  A.ply B.ply            Two mesh paths (or --a / --b)\n"
            << "  --algorithm NAME       vertex (default)\n\n"
            << "Pairwise_distance options:\n"
            << "  -i, --input-dir DIR    Folder containing .ply files\n"
            << "  -o, --output PATH      Matrix file (default: sample_pd/pairwise_distances_<algorithm>.txt)\n"
            << "  --algorithm NAME       vertex (default)\n"
            << "  --max-objects N        Use at most N meshes (default: all)\n"
            << "  --kd-dir DIR           Use prebuilt <stem>.kdtree files for NN search\n\n"
            << "Examples:\n"
            << "  " << program << " gen --format obj\n"
            << "  " << program << " gen --seed 42 --num-objects 5\n"
            << "  " << program << " norm --input-dir sample_gen\n"
            << "  " << program << " kd --input-dir sample_normalized\n"
            << "  " << program << " dist sample_normalized/object_1.ply "
            << "sample_normalized/object_2.ply\n"
            << "  " << program << " pd --input-dir sample_normalized\n";
}

std::optional<AppRequest> parse_app_request(const int argc, char* argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return std::nullopt;
  }

  int option_start = 1;
  AppCommand command = AppCommand::Generate;

  if (!is_option(argv[1])) {
    const std::string token = argv[1];
    if (token == "help" || token == "-h" || token == "--help") {
      print_usage(argv[0]);
      return std::nullopt;
    }
    command = parse_app_command(token);
    option_start = 2;
  }

  AppRequest request;
  request.command = command;

  if (request.command == AppCommand::Generate) {
    const auto config = parse_generate_config(argc - option_start, argv + option_start);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.generate_config = *config;
  } else if (request.command == AppCommand::Normalize) {
    const auto config = parse_normalize_config(argc - option_start, argv + option_start);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.normalize_config = *config;
  } else if (request.command == AppCommand::KdVertices) {
    const auto config = parse_kdvertices_config(argc - option_start, argv + option_start);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.kdvertices_config = *config;
  } else if (request.command == AppCommand::Distance) {
    const auto config = parse_distance_config(argc - option_start, argv + option_start);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.distance_config = *config;
  } else if (request.command == AppCommand::PairwiseDistance) {
    const auto config =
        parse_pairwise_distance_config(argc - option_start, argv + option_start);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.pairwise_distance_config = *config;
  }
  return request;
}

int run_app(const AppRequest& request) {
  switch (request.command) {
    case AppCommand::Generate:
      return run_generate(request.generate_config);
    case AppCommand::Normalize:
      return run_normalize(request.normalize_config);
    case AppCommand::KdVertices:
      return run_kdvertices(request.kdvertices_config);
    case AppCommand::Distance:
      return run_distance(request.distance_config);
    case AppCommand::PairwiseDistance:
      return run_pairwise_distance(request.pairwise_distance_config);
  }
  throw std::logic_error("Unhandled app command.");
}

}  // namespace tin_gen
