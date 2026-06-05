#pragma once

#include "tin_gen/kd_tree.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
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

/// Generic nearest-index variant (KD-tree, R*-tree, etc.).
template <typename SpatialIndex>
[[nodiscard]] double mean_root_mean_square_distance_index(
    const std::vector<KdTree3d::Point>& query_vertices, const SpatialIndex& target_tree) {
  if (query_vertices.empty()) {
    throw std::runtime_error("mean_root_mean_square_distance_index: query_vertices is empty");
  }
  double sum_sq = 0.0;
  for (const auto& u : query_vertices) {
    sum_sq += target_tree.nearest_squared_distance(u);
  }
  return std::sqrt(sum_sq / static_cast<double>(query_vertices.size()));
}

template <typename SpatialIndex>
[[nodiscard]] VertexDistanceResult symmetric_vertex_distance_index(
    const std::vector<KdTree3d::Point>& vertices_a, const SpatialIndex& tree_b,
    const std::vector<KdTree3d::Point>& vertices_b, const SpatialIndex& tree_a) {
  VertexDistanceResult result;
  result.d_a = mean_root_mean_square_distance_index(vertices_a, tree_b);
  result.d_b = mean_root_mean_square_distance_index(vertices_b, tree_a);
  result.distance = 0.5 * (result.d_a + result.d_b);
  return result;
}

}  // namespace tin_gen
