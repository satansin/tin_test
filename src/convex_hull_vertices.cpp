#include "tin_gen/convex_hull_vertices.hpp"

#include "tin_gen/convex_hull_3d.hpp"

#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace tin_gen {
namespace {

std::size_t count_convex_hull_vertices(const std::vector<std::array<double, 3>>& points) {
  return convex_hull_vertex_count(points);
}

void append_initial_tetrahedron(std::vector<std::array<double, 3>>& points, const double scale) {
  points.push_back({0.0, 0.0, 0.0});
  points.push_back({scale, 0.0, 0.0});
  points.push_back({0.0, scale, 0.0});
  points.push_back({0.0, 0.0, scale});
}

std::array<double, 3> random_unit_direction(std::mt19937& rng) {
  std::uniform_real_distribution<double> uni(-1.0, 1.0);
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double len2 = 0.0;
  do {
    x = uni(rng);
    y = uni(rng);
    z = uni(rng);
    len2 = x * x + y * y + z * z;
  } while (len2 < 1e-12);
  const double inv = 1.0 / std::sqrt(len2);
  return {x * inv, y * inv, z * inv};
}

void add_exterior_support_point(std::vector<std::array<double, 3>>& points, const double scale,
                                std::mt19937& rng) {
  const auto dir = random_unit_direction(rng);
  double max_proj = -std::numeric_limits<double>::infinity();
  for (const auto& p : points) {
    max_proj = std::max(max_proj, p[0] * dir[0] + p[1] * dir[1] + p[2] * dir[2]);
  }
  const double margin = scale * 1e-3;
  const double t = max_proj + margin;
  points.push_back({dir[0] * t, dir[1] * t, dir[2] * t});
}

void remove_random_hull_input_point(std::vector<std::array<double, 3>>& points,
                                    std::mt19937& rng) {
  const std::vector<std::size_t> hull_indices = convex_hull_point_indices(points);
  std::uniform_int_distribution<std::size_t> pick(0, hull_indices.size() - 1);
  const std::size_t erase_index = hull_indices[pick(rng)];
  points.erase(points.begin() + static_cast<std::ptrdiff_t>(erase_index));
}

}  // namespace

std::vector<std::array<double, 3>> generate_convex_hull_vertex_points(const std::size_t num_vertices,
                                                                     const double scale,
                                                                     std::mt19937& rng) {
  if (num_vertices < 4) {
    throw std::runtime_error("num_vertices_per_object must be >= 4 for a 3D convex hull.");
  }

  std::vector<std::array<double, 3>> points;
  points.reserve(num_vertices + 8);
  append_initial_tetrahedron(points, scale);

  const std::size_t max_iterations = num_vertices * 32;
  for (std::size_t iter = 0; iter < max_iterations; ++iter) {
    const std::size_t hull_count = count_convex_hull_vertices(points);
    if (hull_count == num_vertices) {
      return points;
    }
    if (hull_count < num_vertices) {
      add_exterior_support_point(points, scale, rng);
      continue;
    }
    if (points.size() <= 4) {
      break;
    }
    remove_random_hull_input_point(points, rng);
  }

  throw std::runtime_error("Failed to generate a convex hull with the requested vertex count.");
}

}  // namespace tin_gen
