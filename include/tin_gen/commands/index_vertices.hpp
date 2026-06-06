#pragma once

#include "tin_gen/config.hpp"

namespace tin_gen {

/// Build KD-tree or R*-tree indexes for every mesh in a folder.
int run_index_vertices(const IndexVerticesConfig& config);

}  // namespace tin_gen
