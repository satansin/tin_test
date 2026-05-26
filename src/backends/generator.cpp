#include "tin_gen/convex_hull_3d.hpp"
#include "tin_gen/convex_hull_vertices.hpp"
#include "tin_gen/generator.hpp"

#include "TriMesh.h"
#include "TriMesh_algo.h"

#include <memory>
#include <random>
#include <stdexcept>

namespace tin_gen {
namespace {

trimesh::TriMesh* tin_to_trimesh(const TinMesh& tin) {
  auto* mesh = new trimesh::TriMesh;
  mesh->vertices.reserve(tin.vertices.size());
  for (const auto& v : tin.vertices) {
    mesh->vertices.emplace_back(static_cast<float>(v[0]), static_cast<float>(v[1]),
                                static_cast<float>(v[2]));
  }
  mesh->faces.reserve(tin.faces.size());
  for (const auto& f : tin.faces) {
    mesh->faces.emplace_back(static_cast<int>(f[0]), static_cast<int>(f[1]),
                             static_cast<int>(f[2]));
  }
  trimesh::orient(mesh);
  return mesh;
}

TinMesh trimesh_to_tin(const trimesh::TriMesh& mesh) {
  TinMesh tin;
  tin.vertices.reserve(mesh.vertices.size());
  for (const auto& v : mesh.vertices) {
    tin.vertices.push_back({v[0], v[1], v[2]});
  }
  tin.faces.reserve(mesh.faces.size());
  for (const auto& f : mesh.faces) {
    tin.faces.push_back({static_cast<std::size_t>(f[0]), static_cast<std::size_t>(f[1]),
                         static_cast<std::size_t>(f[2])});
  }
  return tin;
}

TinMesh convex_hull_from_points(const std::vector<std::array<double, 3>>& points) {
  TinMesh tin = convex_hull_3d(points);
  const std::unique_ptr<trimesh::TriMesh> mesh(tin_to_trimesh(tin));
  mesh->need_neighbors();
  tin = trimesh_to_tin(*mesh);
  if (!tin.is_watertight()) {
    throw std::runtime_error(
        "Generated mesh is not watertight. Check vertex generation.");
  }
  return tin;
}

}  // namespace

std::vector<TinMesh> generate_random_tin(const std::size_t num_objects,
                                         const std::size_t num_vertices_per_object,
                                         const double scale, const unsigned random_seed) {
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
    const auto points =
        generate_convex_hull_vertex_points(num_vertices_per_object, scale, rng);
    TinMesh tin = convex_hull_from_points(points);
    if (tin.vertices.size() != num_vertices_per_object) {
      throw std::runtime_error("Convex hull vertex count mismatch after generation.");
    }
    objects.push_back(std::move(tin));
  }

  return objects;
}

}  // namespace tin_gen
