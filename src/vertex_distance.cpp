#include "tin_gen/vertex_distance.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tin_gen {

double mean_root_mean_square_distance(const std::vector<KdTree3d::Point>& query_vertices,
                                       const KdTree3d& target_tree) {
  if (query_vertices.empty()) {
    throw std::runtime_error("mean_root_mean_square_distance: query_vertices is empty");
  }

  double sum_sq = 0.0;
  for (const auto& u : query_vertices) {
    sum_sq += target_tree.nearest_squared_distance(u);
  }
  return std::sqrt(sum_sq / static_cast<double>(query_vertices.size()));
}

VertexDistanceResult symmetric_vertex_distance(
    const std::vector<KdTree3d::Point>& vertices_a, const KdTree3d& kd_tree_b,
    const std::vector<KdTree3d::Point>& vertices_b, const KdTree3d& kd_tree_a) {
  VertexDistanceResult result;
  result.d_a = mean_root_mean_square_distance(vertices_a, kd_tree_b);
  result.d_b = mean_root_mean_square_distance(vertices_b, kd_tree_a);
  result.distance = 0.5 * (result.d_a + result.d_b);
  return result;
}

double mean_nearest_distance(const std::vector<KdTree3d::Point>& query_points,
                             const KdTree3d& target_tree) {
  if (query_points.empty()) {
    throw std::runtime_error("mean_nearest_distance: query_points is empty");
  }

  double sum = 0.0;
  for (const auto& u : query_points) {
    sum += std::sqrt(target_tree.nearest_squared_distance(u));
  }
  return sum / static_cast<double>(query_points.size());
}

VertexDistanceResult symmetric_chamfer_distance(
    const std::vector<KdTree3d::Point>& points_a, const KdTree3d& kd_tree_b,
    const std::vector<KdTree3d::Point>& points_b, const KdTree3d& kd_tree_a) {
  VertexDistanceResult result;
  result.d_a = mean_nearest_distance(points_a, kd_tree_b);
  result.d_b = mean_nearest_distance(points_b, kd_tree_a);
  result.distance = 0.5 * (result.d_a + result.d_b);
  return result;
}

double max_nearest_distance(const std::vector<KdTree3d::Point>& query_points,
                            const KdTree3d& target_tree) {
  if (query_points.empty()) {
    throw std::runtime_error("max_nearest_distance: query_points is empty");
  }

  double max_dist = 0.0;
  for (const auto& u : query_points) {
    max_dist = std::max(max_dist, std::sqrt(target_tree.nearest_squared_distance(u)));
  }
  return max_dist;
}

VertexDistanceResult symmetric_hausdorff_distance(
    const std::vector<KdTree3d::Point>& points_a, const KdTree3d& kd_tree_b,
    const std::vector<KdTree3d::Point>& points_b, const KdTree3d& kd_tree_a) {
  VertexDistanceResult result;
  result.d_a = max_nearest_distance(points_a, kd_tree_b);
  result.d_b = max_nearest_distance(points_b, kd_tree_a);
  result.distance = std::max(result.d_a, result.d_b);
  return result;
}

}  // namespace tin_gen
