#pragma once

#include "tin_gen/tin_mesh.hpp"

#include <vector>

namespace tin_gen {

/// 3D convex hull as a closed triangle mesh (Qhull).
[[nodiscard]] TinMesh convex_hull_3d(const std::vector<std::array<double, 3>>& points);

}  // namespace tin_gen
