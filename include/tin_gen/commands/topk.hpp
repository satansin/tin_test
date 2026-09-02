#pragma once

#include "tin_gen/config.hpp"

namespace tin_gen {

/// Linear-scan top-k retrieval for one query against a dataset folder/pack.
int run_topk(const TopKConfig& config);

}  // namespace tin_gen
