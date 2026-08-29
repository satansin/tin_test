#include "tin_gen/tin_mesh.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace tin_gen {

namespace {

enum class PlyFormat { Ascii, BinaryLittleEndian, BinaryBigEndian };

template <typename T>
T byteswap(T v) {
  static_assert(std::is_trivially_copyable_v<T>);
  std::array<std::uint8_t, sizeof(T)> b{};
  std::memcpy(b.data(), &v, sizeof(T));
  std::reverse(b.begin(), b.end());
  std::memcpy(&v, b.data(), sizeof(T));
  return v;
}

template <typename T>
T read_binary(std::istream& in, const bool swap_endian) {
  T v{};
  in.read(reinterpret_cast<char*>(&v), sizeof(T));
  if (!in) {
    throw std::runtime_error("Unexpected EOF while reading binary PLY");
  }
  if (swap_endian && sizeof(T) > 1) {
    v = byteswap(v);
  }
  return v;
}

double read_scalar_as_double(std::istream& in, const std::string& type, const bool swap_endian) {
  if (type == "float" || type == "float32") {
    return static_cast<double>(read_binary<float>(in, swap_endian));
  }
  if (type == "double" || type == "float64") {
    return read_binary<double>(in, swap_endian);
  }
  if (type == "char" || type == "int8") {
    return static_cast<double>(read_binary<std::int8_t>(in, false));
  }
  if (type == "uchar" || type == "uint8") {
    return static_cast<double>(read_binary<std::uint8_t>(in, false));
  }
  if (type == "short" || type == "int16") {
    return static_cast<double>(read_binary<std::int16_t>(in, swap_endian));
  }
  if (type == "ushort" || type == "uint16") {
    return static_cast<double>(read_binary<std::uint16_t>(in, swap_endian));
  }
  if (type == "int" || type == "int32") {
    return static_cast<double>(read_binary<std::int32_t>(in, swap_endian));
  }
  if (type == "uint" || type == "uint32") {
    return static_cast<double>(read_binary<std::uint32_t>(in, swap_endian));
  }
  throw std::runtime_error("Unsupported PLY scalar type: " + type);
}

std::size_t read_index_as_size_t(std::istream& in, const std::string& type, const bool swap_endian) {
  if (type == "char" || type == "int8") {
    return static_cast<std::size_t>(read_binary<std::int8_t>(in, false));
  }
  if (type == "uchar" || type == "uint8") {
    return static_cast<std::size_t>(read_binary<std::uint8_t>(in, false));
  }
  if (type == "short" || type == "int16") {
    return static_cast<std::size_t>(read_binary<std::int16_t>(in, swap_endian));
  }
  if (type == "ushort" || type == "uint16") {
    return static_cast<std::size_t>(read_binary<std::uint16_t>(in, swap_endian));
  }
  if (type == "int" || type == "int32") {
    return static_cast<std::size_t>(read_binary<std::int32_t>(in, swap_endian));
  }
  if (type == "uint" || type == "uint32") {
    return static_cast<std::size_t>(read_binary<std::uint32_t>(in, swap_endian));
  }
  throw std::runtime_error("Unsupported PLY index type: " + type);
}

struct PlyVertexProp {
  std::string name;
  std::string type;
};

struct PlyFacePropList {
  std::string name;
  std::string count_type;
  std::string value_type;
};

struct PlyFacePropScalar {
  std::string name;
  std::string type;
};

struct PlyFaceProp {
  bool is_list = false;
  PlyFacePropList list{};
  PlyFacePropScalar scalar{};
};

void triangulate_face_indices(const std::vector<std::size_t>& idx, TinMesh& mesh) {
  if (idx.size() < 3) {
    return;
  }
  const std::size_t v0 = idx[0];
  for (std::size_t k = 1; k + 1 < idx.size(); ++k) {
    mesh.faces.push_back({v0, idx[k], idx[k + 1]});
  }
}

}  // namespace

class PlyMemoryStreamBuf : public std::streambuf {
 public:
  PlyMemoryStreamBuf(const char* data, const std::size_t size) {
    char* begin = const_cast<char*>(data);
    setg(begin, begin, begin + static_cast<std::ptrdiff_t>(size));
  }
};

TinMesh read_ply_memory(const char* data, const std::size_t size, const std::string_view context,
                        const PlyReadContent content) {
  PlyMemoryStreamBuf buffer(data, size);
  std::istream in(&buffer);
  return read_ply_stream(in, context, content);
}

TinMesh read_ply_stream(std::istream& in, const std::string_view context,
                        const PlyReadContent content) {
  const std::string context_str(context);

  std::string line;
  if (!std::getline(in, line) || line != "ply") {
    throw std::runtime_error("Not a PLY file (missing 'ply' header): " + context_str);
  }

  std::size_t vertex_count = 0;
  std::size_t face_count = 0;
  PlyFormat format = PlyFormat::Ascii;
  std::vector<PlyVertexProp> vertex_props;
  std::vector<PlyFaceProp> face_props;
  bool in_vertex_element = false;
  bool in_face_element = false;

  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line == "end_header") {
      break;
    }
    if (line.rfind("comment", 0) == 0) {
      continue;
    }
    if (line.rfind("format", 0) == 0) {
      if (line.find("ascii") != std::string::npos) {
        format = PlyFormat::Ascii;
      } else if (line.find("binary_little_endian") != std::string::npos) {
        format = PlyFormat::BinaryLittleEndian;
      } else if (line.find("binary_big_endian") != std::string::npos) {
        format = PlyFormat::BinaryBigEndian;
      } else {
        throw std::runtime_error("Unsupported PLY format: " + line + " (" + context_str + ")");
      }
      continue;
    }

    std::istringstream iss(line);
    std::string token;
    iss >> token;
    if (token == "element") {
      std::string element_name;
      std::size_t count = 0;
      iss >> element_name >> count;
      in_vertex_element = (element_name == "vertex");
      in_face_element = (element_name == "face");
      if (in_vertex_element) {
        vertex_props.clear();
      }
      if (in_face_element) {
        face_props.clear();
      }
      if (in_vertex_element) {
        vertex_count = count;
      } else if (in_face_element) {
        face_count = count;
      }
    } else if (token == "property") {
      if (in_vertex_element) {
        std::string type;
        std::string name;
        iss >> type >> name;
        if (!type.empty() && !name.empty()) {
          vertex_props.push_back({name, type});
        }
      } else if (in_face_element) {
        std::string maybe_list;
        iss >> maybe_list;
        if (maybe_list == "list") {
          std::string count_type;
          std::string value_type;
          std::string name;
          iss >> count_type >> value_type >> name;
          if (!count_type.empty() && !value_type.empty() && !name.empty()) {
            PlyFaceProp prop;
            prop.is_list = true;
            prop.list = PlyFacePropList{name, count_type, value_type};
            face_props.push_back(prop);
          }
        } else if (!maybe_list.empty()) {
          std::string name;
          iss >> name;
          if (!name.empty()) {
            PlyFaceProp prop;
            prop.is_list = false;
            prop.scalar = PlyFacePropScalar{name, maybe_list};
            face_props.push_back(prop);
          }
        }
      }
    }
  }

  if (vertex_count == 0) {
    throw std::runtime_error("PLY has 0 vertices: " + context_str);
  }

  TinMesh mesh;
  mesh.vertices.reserve(vertex_count);
  if (content == PlyReadContent::Full) {
    mesh.faces.reserve(face_count);
  }

  const auto find_prop_index = [&](const std::string& name) -> std::optional<std::size_t> {
    for (std::size_t i = 0; i < vertex_props.size(); ++i) {
      if (vertex_props[i].name == name) {
        return i;
      }
    }
    return std::nullopt;
  };

  const auto ix = find_prop_index("x");
  const auto iy = find_prop_index("y");
  const auto iz = find_prop_index("z");
  if (!ix || !iy || !iz) {
    throw std::runtime_error("PLY vertex properties must include x,y,z: " + context_str);
  }

  if (format == PlyFormat::Ascii) {
    for (std::size_t i = 0; i < vertex_count; ++i) {
      if (!std::getline(in, line)) {
        throw std::runtime_error("Unexpected EOF while reading vertices: " + context_str);
      }
      std::istringstream iss(line);

      // Read enough scalars to cover x,y,z positions; ignore others.
      double x = 0.0;
      double y = 0.0;
      double z = 0.0;
      for (std::size_t p = 0; p < vertex_props.size(); ++p) {
        double v = 0.0;
        if (!(iss >> v)) {
          throw std::runtime_error("Failed to parse vertex line: " + context_str);
        }
        if (p == *ix) x = v;
        if (p == *iy) y = v;
        if (p == *iz) z = v;
      }
      mesh.vertices.push_back({x, y, z});
    }

    if (content == PlyReadContent::VerticesOnly) {
      for (std::size_t i = 0; i < face_count; ++i) {
        if (!std::getline(in, line)) {
          throw std::runtime_error("Unexpected EOF while skipping faces: " + context_str);
        }
      }
    } else {
      for (std::size_t i = 0; i < face_count; ++i) {
        if (!std::getline(in, line)) {
          throw std::runtime_error("Unexpected EOF while reading faces: " + context_str);
        }
        std::istringstream iss(line);
        for (const auto& prop : face_props) {
          if (prop.is_list) {
            int n = 0;
            if (!(iss >> n)) {
              throw std::runtime_error("Failed to parse face list count: " + context_str);
            }
            if (prop.list.name == "vertex_indices" || prop.list.name == "vertex_index") {
              std::vector<std::size_t> idx;
              if (n > 0) {
                idx.reserve(static_cast<std::size_t>(n));
              }
              for (int k = 0; k < n; ++k) {
                std::size_t v = 0;
                if (!(iss >> v)) {
                  throw std::runtime_error("Failed to parse face indices: " + context_str);
                }
                idx.push_back(v);
              }
              triangulate_face_indices(idx, mesh);
            } else {
              double v = 0.0;
              for (int k = 0; k < n; ++k) {
                if (!(iss >> v)) {
                  throw std::runtime_error("Failed to parse face list values: " + context_str);
                }
              }
            }
          } else {
            double v = 0.0;
            if (!(iss >> v)) {
              throw std::runtime_error("Failed to parse face property: " + context_str);
            }
          }
        }
      }
    }
  } else {
    const bool swap_endian = (format == PlyFormat::BinaryBigEndian);

    for (std::size_t i = 0; i < vertex_count; ++i) {
      double x = 0.0;
      double y = 0.0;
      double z = 0.0;
      for (std::size_t p = 0; p < vertex_props.size(); ++p) {
        const auto& prop = vertex_props[p];
        const double v = read_scalar_as_double(in, prop.type, swap_endian);
        if (p == *ix) x = v;
        if (p == *iy) y = v;
        if (p == *iz) z = v;
      }
      mesh.vertices.push_back({x, y, z});
    }

    if (face_count > 0) {
      if (content == PlyReadContent::VerticesOnly) {
        for (std::size_t i = 0; i < face_count; ++i) {
          for (const auto& prop : face_props) {
            if (prop.is_list) {
              const std::size_t n = read_index_as_size_t(in, prop.list.count_type, swap_endian);
              for (std::size_t k = 0; k < n; ++k) {
                (void)read_index_as_size_t(in, prop.list.value_type, swap_endian);
              }
            } else {
              (void)read_scalar_as_double(in, prop.scalar.type, swap_endian);
            }
          }
        }
      } else {
        for (std::size_t i = 0; i < face_count; ++i) {
          for (const auto& prop : face_props) {
            if (prop.is_list) {
              const std::size_t n = read_index_as_size_t(in, prop.list.count_type, swap_endian);
              if (prop.list.name == "vertex_indices" || prop.list.name == "vertex_index") {
                std::vector<std::size_t> idx;
                if (n > 0) {
                  idx.reserve(n);
                }
                for (std::size_t k = 0; k < n; ++k) {
                  idx.push_back(read_index_as_size_t(in, prop.list.value_type, swap_endian));
                }
                triangulate_face_indices(idx, mesh);
              } else {
                for (std::size_t k = 0; k < n; ++k) {
                  (void)read_scalar_as_double(in, prop.list.value_type, swap_endian);
                }
              }
            } else {
              (void)read_scalar_as_double(in, prop.scalar.type, swap_endian);
            }
          }
        }
      }
    }
  }

  return mesh;
}

TinMesh read_ply(const std::string& filepath, const PlyReadContent content) {
  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open PLY file for reading: " + filepath);
  }
  return read_ply_stream(in, filepath, content);
}

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

void write_ply_stream(std::ostream& out, const TinMesh& mesh) {
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

void write_ply(const std::string& filepath, const TinMesh& mesh) {
  std::ofstream out(filepath);
  if (!out) {
    throw std::runtime_error("Failed to open PLY file for writing: " + filepath);
  }
  write_ply_stream(out, mesh);
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
