#include "tin_gen/face_sampling.hpp"

#include "tin_gen/tin_mesh.hpp"

#include <cmath>
#include <random>
#include <stdexcept>
#include <string>

namespace tin_gen {
namespace {

MeshVertex subtract(const MeshVertex& a, const MeshVertex& b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

MeshVertex cross(const MeshVertex& a, const MeshVertex& b) {
  return {
      a[1] * b[2] - a[2] * b[1],
      a[2] * b[0] - a[0] * b[2],
      a[0] * b[1] - a[1] * b[0],
  };
}

double length(const MeshVertex& v) {
  return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

std::mt19937 make_rng(const unsigned seed) {
  std::mt19937 rng;
  if (seed == 0) {
    std::random_device random_device;
    rng.seed(random_device());
  } else {
    rng.seed(seed);
  }
  return rng;
}

}  // namespace

std::optional<std::string> mesh_face_sampling_error(const TinMesh& mesh) {
  if (mesh.vertices.empty()) {
    return "mesh has no vertices";
  }
  if (mesh.faces.empty()) {
    return "mesh has no faces";
  }

  bool has_non_degenerate_face = false;
  for (std::size_t face_index = 0; face_index < mesh.faces.size(); ++face_index) {
    const auto& face = mesh.faces[face_index];
    for (std::size_t corner = 0; corner < face.size(); ++corner) {
      const std::size_t vertex_index = face[corner];
      if (vertex_index >= mesh.vertices.size()) {
        return "face " + std::to_string(face_index) + " corner " + std::to_string(corner) +
               " references invalid vertex " + std::to_string(vertex_index) +
               " (vertex_count=" + std::to_string(mesh.vertices.size()) + ")";
      }
    }

    const MeshVertex edge_a = subtract(mesh.vertices[face[1]], mesh.vertices[face[0]]);
    const MeshVertex edge_b = subtract(mesh.vertices[face[2]], mesh.vertices[face[0]]);
    const double area = 0.5 * length(cross(edge_a, edge_b));
    has_non_degenerate_face = has_non_degenerate_face || area > 0.0;
  }

  if (!has_non_degenerate_face) {
    return "mesh has no non-degenerate faces";
  }
  return std::nullopt;
}

std::vector<MeshVertex> sample_points_on_faces(const TinMesh& mesh,
                                               const std::size_t num_points,
                                               const unsigned random_seed) {
  if (num_points == 0) {
    return {};
  }
  if (const std::optional<std::string> error = mesh_face_sampling_error(mesh)) {
    throw std::runtime_error("sample_points_on_faces: " + *error);
  }

  std::vector<double> face_areas;
  face_areas.reserve(mesh.faces.size());
  for (const auto& face : mesh.faces) {
    const MeshVertex edge_a = subtract(mesh.vertices[face[1]], mesh.vertices[face[0]]);
    const MeshVertex edge_b = subtract(mesh.vertices[face[2]], mesh.vertices[face[0]]);
    face_areas.push_back(0.5 * length(cross(edge_a, edge_b)));
  }

  std::mt19937 rng = make_rng(random_seed);
  std::discrete_distribution<std::size_t> face_distribution(face_areas.begin(), face_areas.end());
  std::uniform_real_distribution<double> unit_distribution(0.0, 1.0);

  std::vector<MeshVertex> samples;
  samples.reserve(num_points);
  for (std::size_t sample_index = 0; sample_index < num_points; ++sample_index) {
    const std::size_t face_index = face_distribution(rng);
    const auto& face = mesh.faces[face_index];
    const MeshVertex& a = mesh.vertices[face[0]];
    const MeshVertex& b = mesh.vertices[face[1]];
    const MeshVertex& c = mesh.vertices[face[2]];

    const double r1 = std::sqrt(unit_distribution(rng));
    const double r2 = unit_distribution(rng);
    const double weight_a = 1.0 - r1;
    const double weight_b = r1 * (1.0 - r2);
    const double weight_c = r1 * r2;
    samples.push_back({
        weight_a * a[0] + weight_b * b[0] + weight_c * c[0],
        weight_a * a[1] + weight_b * b[1] + weight_c * c[1],
        weight_a * a[2] + weight_b * b[2] + weight_c * c[2],
    });
  }

  return samples;
}

}  // namespace tin_gen
