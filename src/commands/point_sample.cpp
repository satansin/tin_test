#include "tin_gen/commands/point_sample.hpp"

#include "tin_gen/config.hpp"
#include "tin_gen/face_sampling.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/ply_merge.hpp"
#include "tin_gen/tin_mesh.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace tin_gen {

int run_point_sample(const PointSampleConfig& config) {
  const std::filesystem::path input_dir(config.input_dir);
  const std::filesystem::path output_dir(config.output_dir);
  if (!std::filesystem::exists(input_dir) || !std::filesystem::is_directory(input_dir)) {
    throw std::runtime_error("ptsample: input_dir is not a directory: " + input_dir.string());
  }

  const std::vector<std::filesystem::path> input_files =
      list_mesh_files_in_directory(input_dir, ply_list_options(config.max_objects));
  std::filesystem::create_directories(output_dir);
  FolderMeshLoadProgress progress(input_files.size(), "ptsample mesh files");
  std::unique_ptr<PlyMergeWriter> pack_writer;
  if (config.pack_output) {
    pack_writer = std::make_unique<PlyMergeWriter>(output_dir, input_dir,
                                                   config.max_meshes_per_bundle);
  }

  for (std::size_t i = 0; i < input_files.size(); ++i) {
    const std::filesystem::path& input_path = input_files[i];
    const TinMesh input_mesh = read_ply(input_path.string());
    require_non_empty_mesh(input_mesh, "ptsample", input_path);

    const unsigned seed =
        config.random_seed == 0 ? 0 : config.random_seed + static_cast<unsigned>(i);
    const std::vector<MeshVertex> samples =
        sample_points_on_faces(input_mesh, config.num_points, seed);

    TinMesh output_mesh;
    output_mesh.vertices = samples;
    if (pack_writer) {
      pack_writer->add(input_path.filename().string(), output_mesh);
    } else {
      std::filesystem::path output_path = output_dir / input_path.filename();
      output_path.replace_extension(mesh_format_extension(config.format));
      write_mesh(output_path.string(), output_mesh, config.format);
    }
    progress.mark_loaded(i + 1);
  }

  if (pack_writer) {
    const PlyMergeResult result = pack_writer->finish();
    std::cout << "Sampled " << config.num_points << " points per mesh from "
              << result.mesh_count << " TIN(s) directly into " << result.bundle_count
              << " bundle(s) in " << output_dir.string() << " (manifest="
              << result.manifest_path.string() << ")\n";
  } else {
    std::cout << "Sampled " << config.num_points << " points per mesh from "
              << input_files.size() << " TIN(s) to " << output_dir.string()
              << " (format=" << mesh_format_name(config.format) << ")\n";
  }
  return EXIT_SUCCESS;
}

}  // namespace tin_gen
