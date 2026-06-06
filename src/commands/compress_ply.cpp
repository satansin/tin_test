#include "tin_gen/commands/compress_ply.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/ply_merge.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace tin_gen {
namespace fs = std::filesystem;

int run_compress_ply(const CompressPlyConfig& config) {
  const fs::path input_dir(config.input_dir);
  const fs::path output_dir(config.output_dir);

  if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
    throw std::runtime_error("compress: input_dir is not a directory: " + input_dir.string());
  }

  PlyMergeOptions opts;
  opts.max_meshes_per_bundle = config.max_meshes_per_bundle;
  opts.max_objects = config.max_objects;

  CpuTimer cpu_merge;
  WallTimer wall_merge;
  cpu_merge.start();
  wall_merge.start();
  const PlyMergeResult result = write_ply_merge(input_dir, output_dir, opts);
  cpu_merge.stop();
  wall_merge.stop();

  std::cout << "compress\n"
            << "  input: " << input_dir.string() << " (" << result.mesh_count << " meshes)\n"
            << "  output: " << output_dir.string() << " (" << result.bundle_count
            << " bundle file(s))\n"
            << "  manifest: " << result.manifest_path.string() << '\n'
            << "  max_meshes_per_bundle: " << config.max_meshes_per_bundle << '\n';
  print_cpu_wall_timing("compress merge ply files", cpu_merge, wall_merge);

  return EXIT_SUCCESS;
}

}  // namespace tin_gen
