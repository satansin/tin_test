#pragma once

#include "tin_gen/kd_tree.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace tin_gen {

struct VertexDistanceResult {
  double d_a = 0.0;    // directed distance from A points to B
  double d_b = 0.0;    // directed distance from B points to A
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

/// mean_u min_{v in target} ||u - v|| for query points vs target_tree (classic Chamfer).
[[nodiscard]] double mean_nearest_distance(const std::vector<KdTree3d::Point>& query_points,
                                           const KdTree3d& target_tree);

/// Symmetric classic Chamfer using KD-trees on each point cloud.
[[nodiscard]] VertexDistanceResult symmetric_chamfer_distance(
    const std::vector<KdTree3d::Point>& points_a, const KdTree3d& kd_tree_b,
    const std::vector<KdTree3d::Point>& points_b, const KdTree3d& kd_tree_a);

template <typename SpatialIndex>
[[nodiscard]] double mean_nearest_distance_index(const std::vector<KdTree3d::Point>& query_points,
                                                 const SpatialIndex& target_tree) {
  if (query_points.empty()) {
    throw std::runtime_error("mean_nearest_distance_index: query_points is empty");
  }
  double sum = 0.0;
  for (const auto& u : query_points) {
    sum += std::sqrt(target_tree.nearest_squared_distance(u));
  }
  return sum / static_cast<double>(query_points.size());
}

template <typename SpatialIndex>
[[nodiscard]] VertexDistanceResult symmetric_chamfer_distance_index(
    const std::vector<KdTree3d::Point>& points_a, const SpatialIndex& tree_b,
    const std::vector<KdTree3d::Point>& points_b, const SpatialIndex& tree_a) {
  VertexDistanceResult result;
  result.d_a = mean_nearest_distance_index(points_a, tree_b);
  result.d_b = mean_nearest_distance_index(points_b, tree_a);
  result.distance = 0.5 * (result.d_a + result.d_b);
  return result;
}

/// max_u min_{v in target} ||u - v|| for query points vs target_tree (directed Hausdorff).
[[nodiscard]] double max_nearest_distance(const std::vector<KdTree3d::Point>& query_points,
                                          const KdTree3d& target_tree);

/// Symmetric Hausdorff using KD-trees on each point cloud: max(d_A, d_B).
[[nodiscard]] VertexDistanceResult symmetric_hausdorff_distance(
    const std::vector<KdTree3d::Point>& points_a, const KdTree3d& kd_tree_b,
    const std::vector<KdTree3d::Point>& points_b, const KdTree3d& kd_tree_a);

template <typename SpatialIndex>
[[nodiscard]] double max_nearest_distance_index(const std::vector<KdTree3d::Point>& query_points,
                                                const SpatialIndex& target_tree) {
  if (query_points.empty()) {
    throw std::runtime_error("max_nearest_distance_index: query_points is empty");
  }
  double max_dist = 0.0;
  for (const auto& u : query_points) {
    max_dist = std::max(max_dist, std::sqrt(target_tree.nearest_squared_distance(u)));
  }
  return max_dist;
}

template <typename SpatialIndex>
[[nodiscard]] VertexDistanceResult symmetric_hausdorff_distance_index(
    const std::vector<KdTree3d::Point>& points_a, const SpatialIndex& tree_b,
    const std::vector<KdTree3d::Point>& points_b, const SpatialIndex& tree_a) {
  VertexDistanceResult result;
  result.d_a = max_nearest_distance_index(points_a, tree_b);
  result.d_b = max_nearest_distance_index(points_b, tree_a);
  result.distance = std::max(result.d_a, result.d_b);
  return result;
}

}  // namespace tin_gen
