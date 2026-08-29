#pragma once

#include "tin_gen/config.hpp"

#include <optional>

namespace tin_gen {

enum class AppCommand {
  Generate,
  Normalize,
  Compress,
  Kd,
  Rs,
  Distance,
  PointSample,
  PairwiseDistance,
  Validate
};

struct AppRequest {
  AppCommand command = AppCommand::Generate;
  GenerationConfig generate_config;
  NormalizeConfig normalize_config;
  CompressConfig compress_config;
  IndexVerticesConfig index_vertices_config;
  DistanceConfig distance_config;
  PointSampleConfig point_sample_config;
  PairwiseDistanceConfig pairwise_distance_config;
  ValidateConfig validate_config;
};

[[nodiscard]] AppCommand parse_app_command(std::string_view name);
[[nodiscard]] std::string_view app_command_name(AppCommand command);

/// Parse argv into command + options. First non-flag token selects the command
/// (default: generate). Returns nullopt when --help is requested.
[[nodiscard]] std::optional<AppRequest> parse_app_request(int argc, char* argv[]);

void print_usage(const char* program);

/// Dispatch to the handler for the given command.
int run_app(const AppRequest& request);

}  // namespace tin_gen
