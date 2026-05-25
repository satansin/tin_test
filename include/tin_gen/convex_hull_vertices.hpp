#pragma once

#include <array>
#include <cstddef>
#include <random>
#include <vector>

namespace tin_gen {

/// Build a point set whose 3D convex hull has exactly @p num_vertices vertices.
[[nodiscard]] std::vector<std::array<double, 3>> generate_convex_hull_vertex_points(
    std::size_t num_vertices, double scale, std::mt19937& rng);

}  // namespace tin_gen
