#include "tin_gen/mesh_helper.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

std::string to_lower(std::string_view value) {
  std::string out(value);
  std::transform(out.begin(), out.end(), out.begin(),
               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

std::string trim(std::string s) {
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
  while (!s.empty() && is_space(static_cast<unsigned char>(s.front()))) {
    s.erase(s.begin());
  }
  while (!s.empty() && is_space(static_cast<unsigned char>(s.back()))) {
    s.pop_back();
  }
  return s;
}

bool is_all_digits(const std::string_view text) {
  if (text.empty()) {
    return false;
  }
  for (const char c : text) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  return true;
}

}  // namespace

MeshFormat parse_mesh_format(const std::string_view value) {
  const std::string normalized = to_lower(value);
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

std::optional<std::uint64_t> mesh_filename_trailing_index(const std::string_view filename) {
  const std::size_t dot = filename.rfind('.');
  const std::string_view stem = dot == std::string_view::npos ? filename : filename.substr(0, dot);
  const std::size_t underscore = stem.rfind('_');
  if (underscore == std::string_view::npos || underscore + 1 >= stem.size()) {
    return std::nullopt;
  }
  const std::string_view index_text = stem.substr(underscore + 1);
  if (!is_all_digits(index_text)) {
    return std::nullopt;
  }
  return std::stoull(std::string(index_text));
}

bool compare_mesh_paths_by_filename(const fs::path& a, const fs::path& b) {
  const std::string name_a = a.filename().string();
  const std::string name_b = b.filename().string();
  const std::optional<std::uint64_t> index_a = mesh_filename_trailing_index(name_a);
  const std::optional<std::uint64_t> index_b = mesh_filename_trailing_index(name_b);
  if (index_a && index_b && *index_a != *index_b) {
    return *index_a < *index_b;
  }
  return name_a < name_b;
}

void sort_mesh_paths_by_filename(std::vector<fs::path>& paths) {
  std::sort(paths.begin(), paths.end(), compare_mesh_paths_by_filename);
}

std::optional<fs::path> resolve_metadata_path(const fs::path& input_dir,
                                              const ListPlyFilesOptions& opts) {
  if (!opts.use_metadata) {
    return std::nullopt;
  }
  if (opts.metadata_path) {
    const fs::path path = *opts.metadata_path;
    if (!fs::exists(path)) {
      throw std::runtime_error("metadata file not found: " + path.string());
    }
    return path;
  }
  const fs::path default_path = input_dir / "metadata.txt";
  if (fs::exists(default_path)) {
    return default_path;
  }
  return std::nullopt;
}

std::vector<std::string> read_metadata_mesh_list(const fs::path& filepath) {
  std::ifstream in(filepath);
  if (!in) {
    throw std::runtime_error("Failed to open metadata file: " + filepath.string());
  }

  std::vector<std::string> names;
  std::string line;
  bool first = true;
  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (first) {
      first = false;
      if (line.find("mesh_name") != std::string::npos) {
        continue;
      }
    }
    std::istringstream iss(line);
    std::string mesh_name;
    if (!std::getline(iss, mesh_name, ',')) {
      continue;
    }
    mesh_name = trim(mesh_name);
    if (!mesh_name.empty()) {
      names.push_back(mesh_name);
    }
  }
  return names;
}

std::vector<fs::path> list_ply_files_in_directory(const fs::path& input_dir,
                                                  ListPlyFilesOptions opts) {
  std::vector<fs::path> ply_files;
  const std::optional<fs::path> metadata_path = resolve_metadata_path(input_dir, opts);

  if (metadata_path) {
    const std::vector<std::string> metadata_order =
        read_metadata_mesh_list(*metadata_path);
    if (metadata_order.empty()) {
      throw std::runtime_error("metadata file had no mesh entries: " +
                               metadata_path->string());
    }
    for (const auto& filename : metadata_order) {
      const fs::path p = input_dir / filename;
      if (fs::exists(p) && fs::is_regular_file(p) && p.extension() == ".ply") {
        ply_files.push_back(p);
      }
    }
  } else {
    for (const auto& entry : fs::directory_iterator(input_dir)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const fs::path p = entry.path();
      if (p.extension() == ".ply") {
        ply_files.push_back(p);
      }
    }
    sort_mesh_paths_by_filename(ply_files);
  }

  if (ply_files.empty()) {
    throw std::runtime_error("no .ply files found in " + input_dir.string());
  }

  if (opts.max_objects > 0 && ply_files.size() > opts.max_objects) {
    ply_files.resize(opts.max_objects);
  }

  return ply_files;
}

}  // namespace tin_gen
