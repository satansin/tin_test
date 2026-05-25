#include "tin_gen/mesh_format.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace tin_gen {
namespace {

std::string lower(std::string_view value) {
  std::string out(value);
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

}  // namespace

MeshFormat parse_mesh_format(const std::string_view value) {
  const std::string normalized = lower(value);
  if (normalized == "ply") {
    return MeshFormat::Ply;
  }
  if (normalized == "obj") {
    return MeshFormat::Obj;
  }
  throw std::invalid_argument("Unsupported mesh format: " + std::string(value) +
                              " (expected ply or obj)");
}

std::string_view mesh_format_extension(const MeshFormat format) {
  switch (format) {
    case MeshFormat::Ply:
      return ".ply";
    case MeshFormat::Obj:
      return ".obj";
  }
  return ".ply";
}

std::string_view mesh_format_name(const MeshFormat format) {
  switch (format) {
    case MeshFormat::Ply:
      return "ply";
    case MeshFormat::Obj:
      return "obj";
  }
  return "ply";
}

}  // namespace tin_gen
