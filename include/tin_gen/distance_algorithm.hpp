#pragma once

#include <string>
#include <string_view>

namespace tin_gen {

enum class DistanceAlgorithm { Vertex, Chamfer, Hausdorff };

[[nodiscard]] DistanceAlgorithm parse_distance_algorithm(std::string_view name);
[[nodiscard]] std::string_view distance_algorithm_name(DistanceAlgorithm algorithm);

}  // namespace tin_gen
