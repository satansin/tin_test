#pragma once

#include "tin_gen/config.hpp"

namespace tin_gen {

/// Compute all pairwise mesh dissimilarities in a folder and write a distance matrix.
int run_pairwise_distance(const PairwiseDistanceConfig& config);

}  // namespace tin_gen
