#pragma once

#include "tin_gen/config.hpp"

namespace tin_gen {

/// Normalize (zero-center) meshes in a folder and write normalized PLY files.
int run_normalize(const NormalizeConfig& config);

}  // namespace tin_gen

