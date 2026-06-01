#pragma once

#include "tin_gen/kd_tree.hpp"

#include <cstddef>
#include <vector>

namespace tin_gen {

struct VertexDistanceResult {
  double d_a = 0.0;    // RMS distance from A vertices to B
  double d_b = 0.0;    // RMS distance from B vertices to A
  double distance = 0.0;  // (d_a + d_b) / 2
};

/// sqrt( mean_u min_{v in target} ||u - v||^2 ) for query_vertices vs target_tree.
[[nodiscard]] double mean_root_mean_square_distance(
    const std::vector<KdTree3d::Point>& query_vertices, const KdTree3d& target_tree);

/// Symmetric vertex dissimilarity using KD-trees built on each mesh's vertices.
[[nodiscard]] VertexDistanceResult symmetric_vertex_distance(
    const std::vector<KdTree3d::Point>& vertices_a, const KdTree3d& kd_tree_b,
    const std::vector<KdTree3d::Point>& vertices_b, const KdTree3d& kd_tree_a);

}  // namespace tin_gen
