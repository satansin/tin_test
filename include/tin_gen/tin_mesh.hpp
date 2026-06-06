#pragma once

#include "tin_gen/mesh_helper.hpp"

#include <array>
#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace tin_gen {

struct TinMesh {
  std::vector<std::array<double, 3>> vertices;
  std::vector<std::array<std::size_t, 3>> faces;

  [[nodiscard]] bool is_watertight() const;
};

TinMesh read_ply(const std::string& filepath, PlyReadContent content = PlyReadContent::Full);

/// Parse PLY from an open stream. @p context appears in error messages.
[[nodiscard]] TinMesh read_ply_stream(std::istream& in, std::string_view context,
                                    PlyReadContent content = PlyReadContent::Full);

/// Parse PLY from a memory buffer without copying the payload.
[[nodiscard]] TinMesh read_ply_memory(const char* data, std::size_t size, std::string_view context,
                                      PlyReadContent content = PlyReadContent::Full);

void write_mesh(const std::string& filepath, const TinMesh& mesh, MeshFormat format);

}  // namespace tin_gen
