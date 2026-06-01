#include "tin_gen/commands/kdvertices.hpp"

#include "tin_gen/kd_tree.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/tin_mesh.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tin_gen {
namespace fs = std::filesystem;

int run_kdvertices(const KdVerticesConfig& config) {
  const fs::path input_dir(config.input_dir);
  const fs::path output_dir(config.output_dir);

  if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
    throw std::runtime_error("kdvertices: input_dir is not a directory: " + input_dir.string());
  }
  fs::create_directories(output_dir);

  ListPlyFilesOptions list_opts;
  list_opts.max_objects = config.max_objects;

  std::vector<fs::path> ply_files;
  try {
    ply_files = list_ply_files_in_directory(input_dir, list_opts);
  } catch (const std::runtime_error& error) {
    throw std::runtime_error(std::string("kdvertices: ") + error.what());
  }

  std::vector<KdTreeBundleEntry> bundle_entries;
  bundle_entries.reserve(ply_files.size());

  for (const auto& ply_path : ply_files) {
    const TinMesh mesh = read_ply(ply_path.string());
    if (mesh.vertices.empty()) {
      throw std::runtime_error("kdvertices: mesh has no vertices: " + ply_path.string());
    }

    std::vector<KdTree3d::Point> points;
    points.reserve(mesh.vertices.size());
    for (const auto& v : mesh.vertices) {
      points.push_back(v);
    }

    KdTreeBundleEntry entry;
    entry.name = ply_path.stem().string();
    entry.tree = KdTree3d(std::move(points));

    if (config.combined_output) {
      bundle_entries.push_back(std::move(entry));
    } else {
      const fs::path out_path = output_dir / (entry.name + ".kdtree");
      entry.tree.save(out_path.string());
    }
  }

  if (config.combined_output) {
    const fs::path out_path = output_dir / config.combined_file;
    save_kd_tree_bundle(out_path.string(), bundle_entries);
    std::cout << "Built combined KD-tree bundle (" << bundle_entries.size()
              << " trees) at " << out_path.string() << '\n';
  } else {
    std::cout << "Built " << ply_files.size() << " KD-tree index file(s) in "
              << output_dir.string() << '\n';
  }
  return EXIT_SUCCESS;
}

}  // namespace tin_gen
