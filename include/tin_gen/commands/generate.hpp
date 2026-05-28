#pragma once

#include "tin_gen/config.hpp"

namespace tin_gen {

/// Generate random TIN meshes and write them to disk.
int run_generate(const GenerationConfig& config);

}  // namespace tin_gen
