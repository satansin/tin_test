#pragma once

#include "tin_gen/config.hpp"

namespace tin_gen {

/// Merge per-mesh PLY files into `.tinply` bundles and a manifest.
int run_compress(const CompressConfig& config);

}  // namespace tin_gen
