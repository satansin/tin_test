#include "tin_gen/distance_algorithm.hpp"

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

DistanceAlgorithm parse_distance_algorithm(const std::string_view name) {
  const std::string normalized = lower(name);
  if (normalized == "vertex" || normalized == "vertices") {
    return DistanceAlgorithm::Vertex;
  }
  if (normalized == "chamfer") {
    return DistanceAlgorithm::Chamfer;
  }
  if (normalized == "hausdorff" || normalized == "haus") {
    return DistanceAlgorithm::Hausdorff;
  }
  throw std::invalid_argument("Unknown distance algorithm: " + std::string(name));
}

std::string_view distance_algorithm_name(const DistanceAlgorithm algorithm) {
  switch (algorithm) {
    case DistanceAlgorithm::Vertex:
      return "vertex";
    case DistanceAlgorithm::Chamfer:
      return "chamfer";
    case DistanceAlgorithm::Hausdorff:
      return "hausdorff";
  }
  return "vertex";
}

}  // namespace tin_gen
