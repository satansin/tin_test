#include "tin_gen/commands/index_vertices.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/kd_tree.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/rs_tree.hpp"
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

using Point = KdTree3d::Point;

struct IndexVerticesLabels {
  const char* command;
  const char* per_file_label;
  const char* build_label;
};

[[nodiscard]] IndexVerticesLabels labels_for(const VertexIndexKind kind) {
  if (kind == VertexIndexKind::Kd) {
    return {"kd", "per-mesh .kdtree", "kd-trees"};
  }
  return {"rs", "per-mesh .rstree", "r*-trees"};
}

struct MeshPoints {
  std::string name;
  std::vector<Point> points;
};

void save_vertex_indexes(const IndexVerticesConfig& config, const fs::path& output_dir,
                         const std::vector<MeshPoints>& meshes) {
  if (config.kind == VertexIndexKind::Kd) {
    std::vector<KdTreeBundleEntry> built;
    built.reserve(meshes.size());
    for (const auto& mesh : meshes) {
      KdTreeBundleEntry entry;
      entry.name = mesh.name;
      entry.tree = KdTree3d(mesh.points);
      built.push_back(std::move(entry));
    }

    if (config.combined_output) {
      save_kd_tree_bundle((output_dir / config.combined_file).string(), built);
      return;
    }

    for (const auto& entry : built) {
      entry.tree.save((output_dir / (entry.name + ".kdtree")).string());
    }
    return;
  }

  std::vector<RsTreeBundleEntry> built;
  built.reserve(meshes.size());
  for (const auto& mesh : meshes) {
    RsTreeBundleEntry entry;
    entry.name = mesh.name;
    entry.tree = RsTree3d(mesh.points);
    built.push_back(std::move(entry));
  }

  if (config.combined_output) {
    save_rs_tree_bundle((output_dir / config.combined_file).string(), built);
    return;
  }

  for (const auto& entry : built) {
    entry.tree.save((output_dir / (entry.name + ".rstree")).string());
  }
}

}  // namespace

int run_index_vertices(const IndexVerticesConfig& config) {
  const IndexVerticesLabels labels = labels_for(config.kind);
  const fs::path input_dir(config.input_dir);
  const fs::path output_dir(config.output_dir);

  if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
    throw std::runtime_error(std::string(labels.command) +
                             ": input_dir is not a directory: " + input_dir.string());
  }
  fs::create_directories(output_dir);

  const std::string load_label = std::string(labels.command) + " mesh files";

  CpuTimer cpu_read;
  WallTimer wall_read;
  cpu_read.start();
  wall_read.start();
  const LoadedDatasetMeshes loaded = load_all_dataset_meshes(
      input_dir, ply_list_options(config.max_objects), labels.command, load_label);
  cpu_read.stop();
  wall_read.stop();

  std::vector<MeshPoints> meshes;
  meshes.reserve(loaded.meshes.size());
  for (std::size_t i = 0; i < loaded.meshes.size(); ++i) {
    MeshPoints entry;
    entry.name = loaded.listing.paths[i].stem().string();
    entry.points = tin_mesh_vertices(loaded.meshes[i]);
    meshes.push_back(std::move(entry));
  }

  CpuTimer cpu_build;
  WallTimer wall_build;
  cpu_build.start();
  wall_build.start();
  save_vertex_indexes(config, output_dir, meshes);
  cpu_build.stop();
  wall_build.stop();

  std::cout << labels.command << '\n'
            << "  input: " << input_dir.string() << " (" << meshes.size() << " meshes)\n";
  print_dataset_mesh_source(std::cout, loaded.listing);
  std::cout << "  output: " << output_dir.string();
  if (config.combined_output) {
    std::cout << " (" << config.combined_file << ")\n";
  } else {
    std::cout << " (" << labels.per_file_label << ")\n";
  }
  print_cpu_wall_timing(std::string(labels.command) + " read mesh files", cpu_read, wall_read);
  print_cpu_wall_timing(std::string(labels.command) + " build " + labels.build_label, cpu_build,
                        wall_build);

  return EXIT_SUCCESS;
}

}  // namespace tin_gen
