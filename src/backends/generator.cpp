#include "tin_gen/convex_hull_3d.hpp"
#include "tin_gen/convex_hull_vertices.hpp"
#include "tin_gen/generator.hpp"

#include "TriMesh.h"
#include "TriMesh_algo.h"

#include <filesystem>
#include <iostream>
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

unsigned seed_for_object(const unsigned base_seed, const std::size_t object_index) {
  if (base_seed != 0) {
    return base_seed + static_cast<unsigned>(object_index);
  }
  return static_cast<unsigned>(object_index * 2654435761u + 1u);
}

std::mt19937 make_rng(const unsigned seed) {
  std::mt19937 rng;
  if (seed == 0) {
    std::random_device rd;
    rng.seed(rd());
  } else {
    rng.seed(seed);
  }
  return rng;
}

TinMesh generate_single_random_tin_impl(const std::size_t num_vertices_per_object,
                                        const double scale, std::mt19937& rng) {
  const auto points = generate_convex_hull_vertex_points(num_vertices_per_object, scale, rng);
  TinMesh tin = convex_hull_from_points(points);
  if (tin.vertices.size() != num_vertices_per_object) {
    throw std::runtime_error("Convex hull vertex count mismatch after generation.");
  }
  return tin;
}

}  // namespace

TinMesh generate_single_random_tin(const std::size_t num_vertices_per_object, const double scale,
                                   const unsigned random_seed) {
  auto rng = make_rng(random_seed);
  return generate_single_random_tin_impl(num_vertices_per_object, scale, rng);
}

std::vector<TinMesh> generate_random_tin(const std::size_t num_objects,
                                         const std::size_t num_vertices_per_object,
                                         const double scale, const unsigned random_seed) {
  std::vector<TinMesh> objects;
  objects.reserve(num_objects);

  for (std::size_t obj = 0; obj < num_objects; ++obj) {
    auto rng = make_rng(seed_for_object(random_seed, obj));
    objects.push_back(
        generate_single_random_tin_impl(num_vertices_per_object, scale, rng));
  }

  return objects;
}

void generate_and_save_objects(const GenerationConfig& config) {
  std::filesystem::create_directories(config.output_dir);
  const std::string ext(mesh_format_extension(config.format));

  for (std::size_t obj = 0; obj < config.num_objects; ++obj) {
    auto rng = make_rng(seed_for_object(config.random_seed, obj));
    TinMesh tin =
        generate_single_random_tin_impl(config.num_vertices_per_object, config.scale, rng);

    const std::filesystem::path filepath =
        std::filesystem::path(config.output_dir) / ("object_" + std::to_string(obj + 1) + ext);
    write_mesh(filepath.string(), tin, config.format);

    const std::size_t done = obj + 1;
    if (!config.quiet && (done == 1 || done == config.num_objects || done % 1000 == 0)) {
      std::cout << "Generated " << done << " / " << config.num_objects << '\n';
    }
  }

  if (!config.quiet) {
    std::cout << "Wrote " << config.num_objects << " mesh(es) to " << config.output_dir << '\n';
  }
}

}  // namespace tin_gen
