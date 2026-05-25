#include "tin_gen/tin_mesh.hpp"

#include <fstream>
#include <stdexcept>

namespace tin_gen {

bool TinMesh::is_watertight() const {
  if (vertices.empty() || faces.empty()) {
    return false;
  }

  std::vector<int> edge_use(vertices.size() * vertices.size(), 0);
  auto edge_key = [n = vertices.size()](std::size_t a, std::size_t b) {
    if (a > b) {
      std::swap(a, b);
    }
    return static_cast<std::size_t>(a * n + b);
  };

  for (const auto& face : faces) {
    for (int e = 0; e < 3; ++e) {
      const std::size_t i = face[static_cast<std::size_t>(e)];
      const std::size_t j = face[static_cast<std::size_t>((e + 1) % 3)];
      if (i >= vertices.size() || j >= vertices.size()) {
        return false;
      }
      const std::size_t key = edge_key(i, j);
      if (key >= edge_use.size()) {
        return false;
      }
      ++edge_use[key];
    }
  }

  for (const int count : edge_use) {
    if (count != 0 && count != 2) {
      return false;
    }
  }
  return true;
}

void write_ply(const std::string& filepath, const TinMesh& mesh) {
  std::ofstream out(filepath);
  if (!out) {
    throw std::runtime_error("Failed to open PLY file for writing: " + filepath);
  }

  out << "ply\n"
      << "format ascii 1.0\n"
      << "element vertex " << mesh.vertices.size() << '\n'
      << "property float x\n"
      << "property float y\n"
      << "property float z\n"
      << "element face " << mesh.faces.size() << '\n'
      << "property list uchar int vertex_indices\n"
      << "end_header\n";

  for (const auto& v : mesh.vertices) {
    out << v[0] << ' ' << v[1] << ' ' << v[2] << '\n';
  }
  for (const auto& face : mesh.faces) {
    out << "3 " << face[0] << ' ' << face[1] << ' ' << face[2] << '\n';
  }
}

void write_obj(const std::string& filepath, const TinMesh& mesh) {
  std::ofstream out(filepath);
  if (!out) {
    throw std::runtime_error("Failed to open OBJ file for writing: " + filepath);
  }

  for (const auto& v : mesh.vertices) {
    out << "v " << v[0] << ' ' << v[1] << ' ' << v[2] << '\n';
  }
  for (const auto& face : mesh.faces) {
    out << "f " << (face[0] + 1) << ' ' << (face[1] + 1) << ' ' << (face[2] + 1) << '\n';
  }
}

void write_mesh(const std::string& filepath, const TinMesh& mesh, const MeshFormat format) {
  switch (format) {
    case MeshFormat::Ply:
      write_ply(filepath, mesh);
      break;
    case MeshFormat::Obj:
      write_obj(filepath, mesh);
      break;
  }
}

}  // namespace tin_gen
