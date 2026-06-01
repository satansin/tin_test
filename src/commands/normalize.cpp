#include "tin_gen/commands/normalize.hpp"

#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/tin_mesh.hpp"

#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

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

  ListPlyFilesOptions list_opts;
  list_opts.max_objects = config.max_objects;
  list_opts.use_metadata = config.use_metadata;
  if (config.metadata_path) {
    list_opts.metadata_path = fs::path(*config.metadata_path);
  }

  std::vector<fs::path> ply_files;
  try {
    ply_files = list_ply_files_in_directory(input_dir, list_opts);
  } catch (const std::runtime_error& error) {
    throw std::runtime_error(std::string("normalize: ") + error.what());
  }

  const std::optional<fs::path> metadata_path = resolve_metadata_path(input_dir, list_opts);

  FolderMeshLoadProgress load_progress(ply_files.size(), "normalize mesh files");

  std::size_t normalized_count = 0;
  for (std::size_t i = 0; i < ply_files.size(); ++i) {
    const fs::path& ply_path = ply_files[i];
    TinMesh mesh = read_ply(ply_path.string());
    load_progress.mark_loaded(i + 1);
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

