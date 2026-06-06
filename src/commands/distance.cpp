#include "tin_gen/commands/distance.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/distance_algorithm.hpp"
#include "tin_gen/kd_tree.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/rs_tree.hpp"
#include "tin_gen/tin_mesh.hpp"
#include "tin_gen/vertex_distance.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace tin_gen {

namespace {

using Point = KdTree3d::Point;

void print_distance_result(const std::string_view index_name, const VertexDistanceResult& result) {
  std::cout << "  [" << index_name << "] d_A: " << result.d_a << "  d_B: " << result.d_b
            << "  distance: " << result.distance << '\n';
}

int run_distance_vertex_compare(const DistanceConfig& config) {
  const TinMesh mesh_a = read_ply(config.path_a);
  const TinMesh mesh_b = read_ply(config.path_b);

  const std::vector<Point> vertices_a = tin_mesh_vertices(mesh_a);
  const std::vector<Point> vertices_b = tin_mesh_vertices(mesh_b);

  std::cout << "distance compare (algorithm=vertex)\n"
            << "  A: " << config.path_a << " (" << vertices_a.size() << " vertices)\n"
            << "  B: " << config.path_b << " (" << vertices_b.size() << " vertices)\n"
            << "  queries per direction: " << vertices_a.size() << " + " << vertices_b.size()
            << " = " << (vertices_a.size() + vertices_b.size()) << '\n';

  CpuTimer cpu_kd_build;
  WallTimer wall_kd_build;
  cpu_kd_build.start();
  wall_kd_build.start();
  const KdTree3d kd_tree_a(vertices_a);
  const KdTree3d kd_tree_b(vertices_b);
  cpu_kd_build.stop();
  wall_kd_build.stop();
  CpuTimer cpu_rs_build;
  WallTimer wall_rs_build;
  cpu_rs_build.start();
  wall_rs_build.start();
  const RsTree3d rs_tree_a(vertices_a);
  const RsTree3d rs_tree_b(vertices_b);
  cpu_rs_build.stop();
  wall_rs_build.stop();

  CpuTimer cpu_kd_compute;
  WallTimer wall_kd_compute;
  cpu_kd_compute.start();
  wall_kd_compute.start();
  const VertexDistanceResult kd_result =
      symmetric_vertex_distance_index(vertices_a, kd_tree_b, vertices_b, kd_tree_a);
  cpu_kd_compute.stop();
  wall_kd_compute.stop();

  CpuTimer cpu_rs_compute;
  WallTimer wall_rs_compute;
  cpu_rs_compute.start();
  wall_rs_compute.start();
  const VertexDistanceResult rs_result =
      symmetric_vertex_distance_index(vertices_a, rs_tree_b, vertices_b, rs_tree_a);
  cpu_rs_compute.stop();
  wall_rs_compute.stop();

  print_distance_result("kd", kd_result);
  print_distance_result("rs", rs_result);
  print_cpu_wall_timing("kd build", cpu_kd_build, wall_kd_build);
  print_cpu_wall_timing("kd compute", cpu_kd_compute, wall_kd_compute);
  print_cpu_wall_timing("rs build", cpu_rs_build, wall_rs_build);
  print_cpu_wall_timing("rs compute", cpu_rs_compute, wall_rs_compute);

  const double kd_total = cpu_kd_build.elapsed_seconds() + cpu_kd_compute.elapsed_seconds();
  const double rs_total = cpu_rs_build.elapsed_seconds() + cpu_rs_compute.elapsed_seconds();
  std::cout << "  total CPU (build+compute): kd ";
  append_formatted_elapsed_seconds(std::cout, kd_total);
  std::cout << ", rs ";
  append_formatted_elapsed_seconds(std::cout, rs_total);
  if (kd_total > 0.0) {
    std::cout << "  (rs/kd = " << (rs_total / kd_total) << "x)";
  }
  std::cout << '\n';

  return EXIT_SUCCESS;
}

int run_distance_vertex(const DistanceConfig& config) {
  const TinMesh mesh_a = read_ply(config.path_a);
  const TinMesh mesh_b = read_ply(config.path_b);

  const std::vector<Point> vertices_a = tin_mesh_vertices(mesh_a);
  const std::vector<Point> vertices_b = tin_mesh_vertices(mesh_b);

  CpuTimer cpu_build;
  WallTimer wall_build;
  cpu_build.start();
  wall_build.start();
  const RsTree3d rs_tree_a(vertices_a);
  const RsTree3d rs_tree_b(vertices_b);
  cpu_build.stop();
  wall_build.stop();

  CpuTimer cpu_compute;
  WallTimer wall_compute;
  cpu_compute.start();
  wall_compute.start();
  const VertexDistanceResult result =
      symmetric_vertex_distance_index(vertices_a, rs_tree_b, vertices_b, rs_tree_a);
  cpu_compute.stop();
  wall_compute.stop();

  std::cout << "distance (algorithm=vertex)\n"
            << "  A: " << config.path_a << " (" << vertices_a.size() << " vertices)\n"
            << "  B: " << config.path_b << " (" << vertices_b.size() << " vertices)\n"
            << "  d_A: " << result.d_a << '\n'
            << "  d_B: " << result.d_b << '\n'
            << "  distance: " << result.distance << '\n';
  print_cpu_wall_timing("distance rs build", cpu_build, wall_build);
  print_cpu_wall_timing("distance compute", cpu_compute, wall_compute);

  return EXIT_SUCCESS;
}

}  // namespace

int run_distance(const DistanceConfig& config) {
  switch (config.algorithm) {
    case DistanceAlgorithm::Vertex:
      if (config.compare_spatial_index) {
        return run_distance_vertex_compare(config);
      }
      return run_distance_vertex(config);
  }
  throw std::logic_error("distance: unhandled algorithm.");
}

}  // namespace tin_gen
