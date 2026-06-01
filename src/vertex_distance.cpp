#include "tin_gen/vertex_distance.hpp"

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

}  // namespace tin_gen
