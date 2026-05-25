#include "tin_gen/convex_hull_vertices.hpp"
#include "tin_gen/generator.hpp"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/boost/graph/helpers.h>

#include <random>
#include <stdexcept>
#include <unordered_map>

namespace tin_gen {
namespace {

using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point_3 = Kernel::Point_3;
using Surface_mesh = CGAL::Surface_mesh<Point_3>;

TinMesh surface_mesh_to_tin(const Surface_mesh& mesh) {
  TinMesh tin;
  tin.vertices.reserve(mesh.number_of_vertices());
  tin.faces.reserve(mesh.number_of_faces());

  std::unordered_map<Surface_mesh::Vertex_index, std::size_t> vertex_index;
  for (auto v : mesh.vertices()) {
    const auto& p = mesh.point(v);
    vertex_index[v] = tin.vertices.size();
    tin.vertices.push_back(
        {CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z())});
  }

  for (auto f : mesh.faces()) {
    std::size_t i = 0;
    std::array<std::size_t, 3> face{};
    for (auto v : CGAL::vertices_around_face(mesh.halfedge(f), mesh)) {
      if (i < 3) {
        face[i++] = vertex_index.at(v);
      }
    }
    if (i == 3) {
      tin.faces.push_back(face);
    }
  }
  return tin;
}

Surface_mesh convex_hull_mesh(const std::vector<Point_3>& points) {
  Surface_mesh mesh;
  CGAL::convex_hull_3(points.begin(), points.end(), mesh);

  if (mesh.is_empty()) {
    throw std::runtime_error("Convex hull is empty.");
  }
  if (!CGAL::is_triangle_mesh(mesh)) {
    throw std::runtime_error("Convex hull is not a triangle mesh.");
  }
  if (!CGAL::is_closed(mesh)) {
    throw std::runtime_error(
        "Generated mesh is not watertight. Check vertex generation.");
  }
  return mesh;
}

}  // namespace

std::vector<TinMesh> generate_random_tin(const std::size_t num_objects,
                                         const std::size_t num_vertices_per_object,
                                         const double scale,
                                         const unsigned random_seed) {
  std::mt19937 rng;
  if (random_seed == 0) {
    std::random_device rd;
    rng.seed(rd());
  } else {
    rng.seed(random_seed);
  }
  std::vector<TinMesh> objects;
  objects.reserve(num_objects);

  for (std::size_t obj = 0; obj < num_objects; ++obj) {
    const auto vertex_points =
        generate_convex_hull_vertex_points(num_vertices_per_object, scale, rng);
    std::vector<Point_3> points;
    points.reserve(vertex_points.size());
    for (const auto& p : vertex_points) {
      points.emplace_back(p[0], p[1], p[2]);
    }

    Surface_mesh hull = convex_hull_mesh(points);
    if (hull.number_of_vertices() != num_vertices_per_object) {
      throw std::runtime_error("Convex hull vertex count mismatch after generation.");
    }
    TinMesh tin = surface_mesh_to_tin(hull);
    if (!tin.is_watertight()) {
      throw std::runtime_error(
          "Generated mesh is not watertight. Check vertex generation.");
    }
    objects.push_back(std::move(tin));
  }

  return objects;
}

}  // namespace tin_gen
