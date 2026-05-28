#pragma once

#include "tin_gen/tin_mesh.hpp"

#include <vector>

namespace tin_gen {

/// 3D convex hull as a closed triangle mesh (Qhull).
[[nodiscard]] TinMesh convex_hull_3d(const std::vector<std::array<double, 3>>& points);

/// Hull vertex count only (no face triangulation; faster for iterative adjustment).
[[nodiscard]] std::size_t convex_hull_vertex_count(
    const std::vector<std::array<double, 3>>& points);

/// Input point indices that lie on the hull (Qhull vertex ids).
[[nodiscard]] std::vector<std::size_t> convex_hull_point_indices(
    const std::vector<std::array<double, 3>>& points);

}  // namespace tin_gen
