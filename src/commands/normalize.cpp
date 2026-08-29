#include "tin_gen/commands/normalize.hpp"

#include "tin_gen/face_sampling.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/tin_mesh.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
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

  ListMeshFilesOptions list_opts;
  list_opts.max_objects = config.max_objects;
  list_opts.extension = mesh_format_extension(MeshFormat::Ply);

  std::vector<fs::path> ply_files;
  try {
    ply_files = list_mesh_files_in_directory(input_dir, list_opts);
  } catch (const std::runtime_error& error) {
    throw std::runtime_error(std::string("normalize: ") + error.what());
  }

  FolderMeshLoadProgress load_progress(ply_files.size(), "normalize mesh files");

  std::size_t normalized_count = 0;
  std::size_t skipped_count = 0;
  for (std::size_t i = 0; i < ply_files.size(); ++i) {
    const fs::path& ply_path = ply_files[i];
    try {
      TinMesh mesh = read_ply(ply_path.string());
      if (const std::optional<std::string> error = mesh_face_sampling_error(mesh)) {
        std::cerr << "Warning: skipping " << ply_path.filename().string() << ": " << *error
                  << '\n';
        ++skipped_count;
        load_progress.mark_loaded(i + 1);
        continue;
      }

      zero_center(mesh);

      const fs::path out_path = output_dir / ply_path.filename();
      write_mesh(out_path.string(), mesh, MeshFormat::Ply);
      ++normalized_count;
    } catch (const std::exception& error) {
      std::cerr << "Warning: skipping " << ply_path.filename().string() << ": " << error.what()
                << '\n';
      ++skipped_count;
    }
    load_progress.mark_loaded(i + 1);
  }

  std::cout << "Normalized " << normalized_count << " mesh(es) to " << output_dir.string()
            << " (translation only: zero-centered by vertex mean)";
  if (skipped_count > 0) {
    std::cout << "; skipped " << skipped_count << " file(s) due to errors";
  }
  std::cout << '\n';
  return EXIT_SUCCESS;
}

}  // namespace tin_gen
