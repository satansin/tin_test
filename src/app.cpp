#include "tin_gen/app.hpp"

#include "tin_gen/commands/generate.hpp"
#include "tin_gen/commands/distance.hpp"
#include "tin_gen/commands/index_vertices.hpp"
#include "tin_gen/commands/pairwise_distance.hpp"
#include "tin_gen/commands/point_sample.hpp"
#include "tin_gen/commands/normalize.hpp"
#include "tin_gen/commands/compress.hpp"
#include "tin_gen/commands/validate.hpp"

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
  if (normalized == "compress" || normalized == "pack") {
    return AppCommand::Compress;
  }
  if (normalized == "kdvertices" || normalized == "kd") {
    return AppCommand::Kd;
  }
  if (normalized == "rsvertices" || normalized == "rs") {
    return AppCommand::Rs;
  }
  if (normalized == "distance" || normalized == "dist") {
    return AppCommand::Distance;
  }
  if (normalized == "ptsample" || normalized == "point_sample" ||
      normalized == "sample_points") {
    return AppCommand::PointSample;
  }
  if (normalized == "pairwise_distance" || normalized == "pairwisedistance" ||
      normalized == "pd") {
    return AppCommand::PairwiseDistance;
  }
  if (normalized == "validate" || normalized == "check") {
    return AppCommand::Validate;
  }
  throw std::invalid_argument("Unknown command: " + std::string(name));
}

std::string_view app_command_name(const AppCommand command) {
  switch (command) {
    case AppCommand::Generate:
      return "generate";
    case AppCommand::Normalize:
      return "normalize";
    case AppCommand::Compress:
      return "compress";
    case AppCommand::Kd:
      return "kd";
    case AppCommand::Rs:
      return "rs";
    case AppCommand::Distance:
      return "distance";
    case AppCommand::PointSample:
      return "ptsample";
    case AppCommand::PairwiseDistance:
      return "pairwise_distance";
    case AppCommand::Validate:
      return "validate";
  }
  return "generate";
}

void print_usage(const char* program) {
  std::cout << "Usage: " << program << " [command] [options]\n\n"
            << "Commands:\n"
            << "  generate (gen)   Build random TIN meshes and write files\n"
            << "  normalize (norm) Translate PLY meshes to zero mean (no scaling)\n"
            << "  compress (pack)  Merge PLY files into bundle files + manifest\n"
            << "  kd (kdvertices)  Build KD-tree vertex indexes for meshes in a folder\n"
            << "  rs               Build R*-tree vertex indexes for meshes in a folder\n"
            << "  distance (dist)    Dissimilarity between two PLY meshes\n"
            << "  ptsample            Sample points across TIN faces in a folder\n"
            << "  pairwise_distance (pd)  All pairwise dissimilarities in a folder\n"
            << "  validate (check)        Check meshes in a folder for invalid faces/vertices\n"
            << "  help                    Show this help\n\n"
            << "Command aliases: gen, norm, pack, kd, rs, dist, ptsample, pd, check\n\n"
            << "Generate options:\n"
            << "  --format FORMAT     Output mesh format: ply | obj (default: ply)\n"
            << "  -o, --output-dir DIR   Output directory (required)\n"
            << "  --num-objects N     Number of meshes to generate (default: 10)\n"
            << "  --num-vertices-per-object N   Vertices on each convex hull (default: 200)\n"
            << "  --scale VALUE       Random point coordinate scale (default: 1)\n"
            << "  --seed N            RNG seed, 0 = random (default: 0)\n"
            << "  -q, --quiet         Progress every 1000 meshes (not every file)\n\n"
            << "Normalize options:\n"
            << "  -i, --input-dir DIR    Folder containing .ply files (required)\n"
            << "  -o, --output-dir DIR   Output directory (required)\n"
            << "  --max-objects N        Normalize at most N meshes (default: all)\n\n"
            << "Compress options:\n"
            << "  -i, --input-dir DIR    Folder containing .ply files (required)\n"
            << "  -o, --output-dir DIR   Output folder (required)\n"
            << "  --max-meshes-per-bundle N   Split bundles at N meshes (default: 5000)\n"
            << "  --max-objects N        Merge at most N meshes (default: all)\n\n"
            << "Index build options (kd / rs):\n"
            << "  -i, --input-dir DIR    Folder containing .ply files (required)\n"
            << "  -o, --output-dir DIR   Output directory (required)\n"
            << "  --max-objects N        Process at most N meshes (default: all)\n"
            << "  --combined             Write split bundles + manifest (5000 indexes per file)\n\n"
            << "Distance options:\n"
            << "  A.ply B.ply            Two mesh paths (or --a / --b)\n"
            << "  --algorithm NAME       vertex (default)\n"
            << "  --compare-index        Build KD-tree and R*-tree; print side-by-side timings\n\n"
            << "Point-sampling options:\n"
            << "  -i, --input-dir DIR    Input folder containing .ply files (required)\n"
            << "  -o, --output-dir DIR   Output folder for sampled point clouds (required)\n"
            << "  -n, --num-points N     Number of surface points (required, N > 0)\n"
            << "  --max-objects N        Process at most N meshes (default: all)\n"
            << "  --max-meshes-per-bundle N   Bundle limit with --pack (default: 5000)\n"
            << "  --pack                 Write sampled meshes directly as .tinply bundles\n"
            << "  --format FORMAT        Output format: ply | obj (default: ply)\n"
            << "  --seed N               RNG seed, 0 = random (default: 0)\n\n"
            << "Pairwise_distance options:\n"
            << "  -i, --input-dir DIR    Folder containing .ply files (required)\n"
            << "  -o, --output PATH      Matrix file (required)\n"
            << "  --algorithm NAME       vertex (default)\n"
            << "  --max-objects N        Use at most N meshes (default: all)\n"
            << "  --rs-dir DIR, -rs DIR  Use prebuilt <stem>.rstree files for NN search (default index)\n"
            << "  --kd-dir DIR, -kd DIR  Use prebuilt <stem>.kdtree files for NN search\n\n"
            << "Validate options:\n"
            << "  -i, --input-dir DIR    Folder or pack directory to check (required)\n"
            << "  -o, --report PATH      Optional TSV report of invalid meshes\n"
            << "  --max-objects N        Check at most N meshes (default: all)\n\n"
            << "Examples:\n"
            << "  " << program << " gen --format obj --output-dir output/example_obj\n"
            << "  " << program << " gen --seed 42 --num-objects 5 --output-dir output/example\n"
            << "  " << program << " norm --input-dir output/example --output-dir output/normalized\n"
            << "  " << program << " kd --input-dir output/normalized --output-dir output/kd\n"
            << "  " << program << " rs --input-dir output/normalized --output-dir output/rs\n"
            << "  " << program << " dist output/normalized/object_1.ply "
            << "output/normalized/object_2.ply\n"
            << "  " << program << " ptsample -i output/normalized "
            << "-o output/samples -n 10000 --seed 42\n"
            << "  " << program << " pd --input-dir output/normalized "
            << "--output output/pairwise_distances_vertex.txt\n"
            << "  " << program << " validate -i output/normalized "
            << "-o output/validate_report.tsv\n";
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
  } else if (request.command == AppCommand::Compress) {
    const auto config = parse_compress_config(argc - option_start, argv + option_start);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.compress_config = *config;
  } else if (request.command == AppCommand::Kd) {
    const auto config =
        parse_index_vertices_config(argc - option_start, argv + option_start, VertexIndexKind::Kd);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.index_vertices_config = *config;
  } else if (request.command == AppCommand::Rs) {
    const auto config =
        parse_index_vertices_config(argc - option_start, argv + option_start, VertexIndexKind::Rs);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.index_vertices_config = *config;
  } else if (request.command == AppCommand::Distance) {
    const auto config = parse_distance_config(argc - option_start, argv + option_start);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.distance_config = *config;
  } else if (request.command == AppCommand::PointSample) {
    const auto config = parse_point_sample_config(argc - option_start, argv + option_start);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.point_sample_config = *config;
  } else if (request.command == AppCommand::PairwiseDistance) {
    const auto config =
        parse_pairwise_distance_config(argc - option_start, argv + option_start);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.pairwise_distance_config = *config;
  } else if (request.command == AppCommand::Validate) {
    const auto config = parse_validate_config(argc - option_start, argv + option_start);
    if (!config) {
      print_usage(argv[0]);
      return std::nullopt;
    }
    request.validate_config = *config;
  }
  return request;
}

int run_app(const AppRequest& request) {
  switch (request.command) {
    case AppCommand::Generate:
      return run_generate(request.generate_config);
    case AppCommand::Normalize:
      return run_normalize(request.normalize_config);
    case AppCommand::Compress:
      return run_compress(request.compress_config);
    case AppCommand::Kd:
    case AppCommand::Rs:
      return run_index_vertices(request.index_vertices_config);
    case AppCommand::Distance:
      return run_distance(request.distance_config);
    case AppCommand::PointSample:
      return run_point_sample(request.point_sample_config);
    case AppCommand::PairwiseDistance:
      return run_pairwise_distance(request.pairwise_distance_config);
    case AppCommand::Validate:
      return run_validate(request.validate_config);
  }
  throw std::logic_error("Unhandled app command.");
}

}  // namespace tin_gen
