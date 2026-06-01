#pragma once

#include "tin_gen/config.hpp"

namespace tin_gen {

/// Compute dissimilarity between two PLY meshes using the selected algorithm.
int run_distance(const DistanceConfig& config);

}  // namespace tin_gen
