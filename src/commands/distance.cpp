#include "tin_gen/commands/distance.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/distance_algorithm.hpp"
#include "tin_gen/kd_tree.hpp"
#include "tin_gen/tin_mesh.hpp"
#include "tin_gen/vertex_distance.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace tin_gen {

namespace {

std::vector<KdTree3d::Point> mesh_vertices(const TinMesh& mesh) {
  std::vector<KdTree3d::Point> points;
  points.reserve(mesh.vertices.size());
  for (const auto& v : mesh.vertices) {
    points.push_back(v);
  }
  return points;
}

int run_distance_vertex(const DistanceConfig& config) {
  const TinMesh mesh_a = read_ply(config.path_a);
  const TinMesh mesh_b = read_ply(config.path_b);

  const std::vector<KdTree3d::Point> vertices_a = mesh_vertices(mesh_a);
  const std::vector<KdTree3d::Point> vertices_b = mesh_vertices(mesh_b);

  CpuTimer cpu_build;
  WallTimer wall_build;
  cpu_build.start();
  wall_build.start();
  const KdTree3d kd_tree_a(vertices_a);
  const KdTree3d kd_tree_b(vertices_b);
  cpu_build.stop();
  wall_build.stop();

  CpuTimer cpu_compute;
  WallTimer wall_compute;
  cpu_compute.start();
  wall_compute.start();
  const VertexDistanceResult result =
      symmetric_vertex_distance(vertices_a, kd_tree_b, vertices_b, kd_tree_a);
  cpu_compute.stop();
  wall_compute.stop();

  std::cout << "distance (algorithm=vertex)\n"
            << "  A: " << config.path_a << " (" << vertices_a.size() << " vertices)\n"
            << "  B: " << config.path_b << " (" << vertices_b.size() << " vertices)\n"
            << "  d_A: " << result.d_a << '\n'
            << "  d_B: " << result.d_b << '\n'
            << "  distance: " << result.distance << '\n';
  print_cpu_wall_timing("distance kd-tree build", cpu_build, wall_build);
  print_cpu_wall_timing("distance compute", cpu_compute, wall_compute);

  return EXIT_SUCCESS;
}

}  // namespace

int run_distance(const DistanceConfig& config) {
  switch (config.algorithm) {
    case DistanceAlgorithm::Vertex:
      return run_distance_vertex(config);
  }
  throw std::logic_error("Unhandled distance algorithm.");
}

}  // namespace tin_gen
