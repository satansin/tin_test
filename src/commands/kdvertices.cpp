#include "tin_gen/commands/kdvertices.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/kd_tree.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/tin_mesh.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

struct MeshPoints {
  std::string name;
  std::vector<KdTree3d::Point> points;
};

std::vector<KdTree3d::Point> mesh_to_points(const TinMesh& mesh) {
  std::vector<KdTree3d::Point> points;
  points.reserve(mesh.vertices.size());
  for (const auto& v : mesh.vertices) {
    points.push_back(v);
  }
  return points;
}

}  // namespace

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

  std::vector<MeshPoints> meshes;
  meshes.reserve(ply_files.size());

  FolderMeshLoadProgress load_progress(ply_files.size(), "kdvertices mesh files");

  CpuTimer cpu_read;
  WallTimer wall_read;
  cpu_read.start();
  wall_read.start();
  for (std::size_t i = 0; i < ply_files.size(); ++i) {
    const TinMesh mesh = read_ply(ply_files[i].string());
    if (mesh.vertices.empty()) {
      throw std::runtime_error("kdvertices: mesh has no vertices: " +
                               ply_files[i].string());
    }
    MeshPoints entry;
    entry.name = ply_files[i].stem().string();
    entry.points = mesh_to_points(mesh);
    meshes.push_back(std::move(entry));
    load_progress.mark_loaded(i + 1);
  }
  cpu_read.stop();
  wall_read.stop();

  std::vector<KdTreeBundleEntry> built;
  built.reserve(meshes.size());

  CpuTimer cpu_build;
  WallTimer wall_build;
  cpu_build.start();
  wall_build.start();
  for (const auto& mesh : meshes) {
    KdTreeBundleEntry entry;
    entry.name = mesh.name;
    entry.tree = KdTree3d(mesh.points);
    built.push_back(std::move(entry));
  }
  cpu_build.stop();
  wall_build.stop();

  CpuTimer cpu_save;
  WallTimer wall_save;
  cpu_save.start();
  wall_save.start();
  if (config.combined_output) {
    const fs::path out_path = output_dir / config.combined_file;
    save_kd_tree_bundle(out_path.string(), built);
  } else {
    for (const auto& entry : built) {
      const fs::path out_path = output_dir / (entry.name + ".kdtree");
      entry.tree.save(out_path.string());
    }
  }
  cpu_save.stop();
  wall_save.stop();

  std::cout << "kdvertices\n"
            << "  input: " << input_dir.string() << " (" << built.size() << " meshes)\n"
            << "  output: " << output_dir.string();
  if (config.combined_output) {
    std::cout << " (" << config.combined_file << ")\n";
  } else {
    std::cout << " (per-mesh .kdtree)\n";
  }
  print_cpu_wall_timing("kdvertices read mesh files", cpu_read, wall_read);
  print_cpu_wall_timing("kdvertices build kd-trees", cpu_build, wall_build);
  print_cpu_wall_timing("kdvertices save to file", cpu_save, wall_save);

  return EXIT_SUCCESS;
}

}  // namespace tin_gen
