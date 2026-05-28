#include "tin_gen/commands/normalize.hpp"

#include "tin_gen/tin_mesh.hpp"

#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <vector>

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

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

std::unordered_set<std::string> read_metadata_mesh_names(const std::string& filepath) {
  std::ifstream in(filepath);
  if (!in) {
    throw std::runtime_error("Failed to open metadata file: " + filepath);
  }

  std::unordered_set<std::string> names;
  std::string line;
  bool first = true;
  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (first) {
      // optional header
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
      names.insert(mesh_name);
    }
  }
  return names;
}

std::vector<std::string> read_metadata_mesh_list(const std::string& filepath) {
  std::ifstream in(filepath);
  if (!in) {
    throw std::runtime_error("Failed to open metadata file: " + filepath);
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

void zero_center(TinMesh& mesh) {
  if (mesh.vertices.empty()) {
    return;
  }
  std::array<double, 3> mean{0.0, 0.0, 0.0};
  for (const auto& v : mesh.vertices) {
    mean[0] += v[0];
    mean[1] += v[1];
    mean[2] += v[2];
  }
  const double inv_n = 1.0 / static_cast<double>(mesh.vertices.size());
  mean[0] *= inv_n;
  mean[1] *= inv_n;
  mean[2] *= inv_n;

  for (auto& v : mesh.vertices) {
    v[0] -= mean[0];
    v[1] -= mean[1];
    v[2] -= mean[2];
  }
}

std::optional<fs::path> resolve_metadata_path(const NormalizeConfig& config,
                                             const fs::path& input_dir) {
  if (!config.use_metadata) {
    return std::nullopt;
  }
  if (config.metadata_path) {
    const fs::path path(*config.metadata_path);
    if (!fs::exists(path)) {
      throw std::runtime_error("normalize: metadata file not found: " + path.string());
    }
    return path;
  }
  const fs::path default_path = input_dir / "metadata.txt";
  if (fs::exists(default_path)) {
    return default_path;
  }
  return std::nullopt;
}

}  // namespace

int run_normalize(const NormalizeConfig& config) {
  if (config.input_dir.empty()) {
    throw std::runtime_error("normalize: input_dir is required");
  }

  const fs::path input_dir(config.input_dir);
  const fs::path output_dir(config.output_dir);

  if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
    throw std::runtime_error("normalize: input_dir is not a directory: " + input_dir.string());
  }
  fs::create_directories(output_dir);

  const std::optional<fs::path> metadata_path = resolve_metadata_path(config, input_dir);

  std::optional<std::unordered_set<std::string>> whitelist;
  std::vector<std::string> metadata_order;
  if (metadata_path) {
    whitelist = read_metadata_mesh_names(metadata_path->string());
    if (whitelist->empty()) {
      throw std::runtime_error("normalize: metadata file had no mesh entries: " +
                               metadata_path->string());
    }
    metadata_order = read_metadata_mesh_list(metadata_path->string());
  }

  std::vector<fs::path> ply_files;
  if (!metadata_order.empty()) {
    // Follow metadata order, but only include files that exist.
    for (const auto& filename : metadata_order) {
      if (whitelist && whitelist->find(filename) == whitelist->end()) {
        continue;
      }
      const fs::path p = input_dir / filename;
      if (fs::exists(p) && fs::is_regular_file(p)) {
        ply_files.push_back(p);
      }
    }
  } else {
    for (const auto& entry : fs::directory_iterator(input_dir)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const fs::path p = entry.path();
      if (p.extension() != ".ply") {
        continue;
      }
      const std::string filename = p.filename().string();
      if (whitelist && whitelist->find(filename) == whitelist->end()) {
        continue;
      }
      ply_files.push_back(p);
    }
    std::sort(ply_files.begin(), ply_files.end(),
              [](const fs::path& a, const fs::path& b) { return a.filename() < b.filename(); });
  }

  if (ply_files.empty()) {
    throw std::runtime_error("normalize: no .ply files found in " + input_dir.string());
  }

  if (config.max_objects > 0 && ply_files.size() > config.max_objects) {
    ply_files.resize(config.max_objects);
  }

  std::size_t normalized_count = 0;
  for (const auto& ply_path : ply_files) {
    TinMesh mesh = read_ply(ply_path.string());
    zero_center(mesh);

    const fs::path out_path = output_dir / ply_path.filename();
    write_mesh(out_path.string(), mesh, MeshFormat::Ply);
    ++normalized_count;
  }

  if (metadata_path) {
    const fs::path meta_dst = output_dir / metadata_path->filename();
    fs::copy_file(*metadata_path, meta_dst, fs::copy_options::overwrite_existing);
    std::cout << "Copied metadata to " << meta_dst.string() << '\n';
  }

  std::cout << "Normalized " << normalized_count << " mesh(es) to " << output_dir.string()
            << " (translation only: zero-centered by vertex mean)\n";
  return EXIT_SUCCESS;
}

}  // namespace tin_gen

