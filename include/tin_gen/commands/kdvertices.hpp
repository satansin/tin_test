#pragma once

#include "tin_gen/config.hpp"

namespace tin_gen {

/// Build per-mesh KD-tree vertex indexes for all .ply files in a folder.
int run_kdvertices(const KdVerticesConfig& config);

}  // namespace tin_gen
