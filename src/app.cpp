#include "tin_gen/app.hpp"

#include "tin_gen/commands/generate.hpp"
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
  throw std::invalid_argument("Unknown command: " + std::string(name));
}

std::string_view app_command_name(const AppCommand command) {
  switch (command) {
    case AppCommand::Generate:
      return "generate";
    case AppCommand::Normalize:
      return "normalize";
  }
  return "generate";
}

void print_usage(const char* program) {
  std::cout << "Usage: " << program << " [command] [options]\n\n"
            << "Commands:\n"
            << "  generate   Build random TIN meshes and write files\n"
            << "  normalize  Translate PLY meshes to zero mean (no scaling)\n"
            << "  help       Show this help\n\n"
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
            << "  --max-objects N        Normalize at most N meshes (default: all)\n"
            << "  --metadata PATH        Metadata CSV (default: <input-dir>/metadata.txt)\n"
            << "  --no-metadata          Do not read or write metadata\n\n"
            << "Examples:\n"
            << "  " << program << " generate --format obj\n"
            << "  " << program << " generate --seed 42 --num-objects 5\n"
            << "  " << program << " normalize --input-dir sample_gen\n";
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
  }
  return request;
}

int run_app(const AppRequest& request) {
  switch (request.command) {
    case AppCommand::Generate:
      return run_generate(request.generate_config);
    case AppCommand::Normalize:
      return run_normalize(request.normalize_config);
  }
  throw std::logic_error("Unhandled app command.");
}

}  // namespace tin_gen
