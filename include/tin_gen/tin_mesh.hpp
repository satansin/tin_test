#pragma once

#include "tin_gen/mesh_format.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace tin_gen {

struct TinMesh {
  std::vector<std::array<double, 3>> vertices;
  std::vector<std::array<std::size_t, 3>> faces;

  [[nodiscard]] bool is_watertight() const;
};

void write_mesh(const std::string& filepath, const TinMesh& mesh, MeshFormat format);

}  // namespace tin_gen
