#include "tin_gen/commands/index_vertices.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/kd_tree.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/ply_merge.hpp"
#include "tin_gen/rs_tree.hpp"
#include "tin_gen/tin_mesh.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
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

struct TimingTotals {
  double read_cpu_seconds = 0.0;
  double read_wall_seconds = 0.0;
  double build_cpu_seconds = 0.0;
  double build_wall_seconds = 0.0;
};

void accumulate_timing(TimingTotals& totals, const CpuTimer& cpu, const WallTimer& wall) {
  totals.read_cpu_seconds += cpu.elapsed_seconds();
  totals.read_wall_seconds += wall.elapsed_seconds();
}

void accumulate_build_timing(TimingTotals& totals, const CpuTimer& cpu, const WallTimer& wall) {
  totals.build_cpu_seconds += cpu.elapsed_seconds();
  totals.build_wall_seconds += wall.elapsed_seconds();
}

void print_timing_totals(const std::string_view label, const TimingTotals& totals) {
  std::cout << std::fixed << std::setprecision(6);
  std::cout << label << " timing:\n"
            << "  CPU time: " << totals.read_cpu_seconds << " s\n"
            << "  Wall time: " << totals.read_wall_seconds << " s\n";
}

void print_build_timing_totals(const std::string_view label, const TimingTotals& totals) {
  std::cout << std::fixed << std::setprecision(6);
  std::cout << label << " timing:\n"
            << "  CPU time: " << totals.build_cpu_seconds << " s\n"
            << "  Wall time: " << totals.build_wall_seconds << " s\n";
}

void save_kd_vertex_indexes(const IndexVerticesConfig& config, const fs::path& output_dir,
                            const std::vector<KdTreeBundleEntry>& trees) {
  if (config.combined_output) {
    save_kd_tree_bundle((output_dir / config.combined_file).string(), trees);
    return;
  }

  for (const auto& entry : trees) {
    entry.tree.save((output_dir / (entry.name + ".kdtree")).string());
  }
}

void save_rs_vertex_indexes(const IndexVerticesConfig& config, const fs::path& output_dir,
                            const std::vector<RsTreeBundleEntry>& trees) {
  if (config.combined_output) {
    save_rs_tree_bundle((output_dir / config.combined_file).string(), trees);
    return;
  }

  for (const auto& entry : trees) {
    entry.tree.save((output_dir / (entry.name + ".rstree")).string());
  }
}

void build_kd_tree_for_mesh(std::vector<KdTreeBundleEntry>& trees, const std::string& name,
                            std::vector<Point> points) {
  trees.push_back(KdTreeBundleEntry{name, KdTree3d(std::move(points))});
}

void build_rs_tree_for_mesh(std::vector<RsTreeBundleEntry>& trees, const std::string& name,
                            std::vector<Point> points) {
  trees.push_back(RsTreeBundleEntry{name, RsTree3d(std::move(points))});
}

void process_pack_meshes(const IndexVerticesConfig& config, const IndexVerticesLabels& labels,
                         const fs::path& input_dir, const fs::path& manifest_path,
                         DatasetMeshListing& listing, TimingTotals& timings,
                         std::vector<KdTreeBundleEntry>& kd_trees,
                         std::vector<RsTreeBundleEntry>& rs_trees) {
  const std::string load_label = std::string(labels.command) + " mesh files";
  PlyMergeDatasetReader reader(manifest_path, config.max_objects);
  listing = reader.make_listing(input_dir);

  DatasetMeshLoadProgress progress(listing, load_label);
  reader.set_bundle_loaded_callback([&progress](const PackBundleLoadedInfo& info) {
    progress.on_bundle_loaded(info.bundle_file, info.size_bytes, info.read_wall_seconds,
                              info.read_cpu_seconds, info.mesh_index);
  });

  const std::size_t mesh_count = reader.mesh_count();
  if (config.kind == VertexIndexKind::Kd) {
    kd_trees.reserve(mesh_count);
  } else {
    rs_trees.reserve(mesh_count);
  }

  for (std::size_t i = 0; i < mesh_count; ++i) {
    CpuTimer cpu_read;
    WallTimer wall_read;
    cpu_read.start();
    wall_read.start();
    TinMesh mesh = reader.read_mesh(i, PlyReadContent::VerticesOnly);
    cpu_read.stop();
    wall_read.stop();
    accumulate_timing(timings, cpu_read, wall_read);

    require_non_empty_mesh(mesh, labels.command, listing.paths[i]);
    const std::string name = listing.paths[i].stem().string();

    CpuTimer cpu_build;
    WallTimer wall_build;
    cpu_build.start();
    wall_build.start();
    std::vector<Point> points = tin_mesh_vertices(mesh);
    if (config.kind == VertexIndexKind::Kd) {
      build_kd_tree_for_mesh(kd_trees, name, std::move(points));
    } else {
      build_rs_tree_for_mesh(rs_trees, name, std::move(points));
    }
    cpu_build.stop();
    wall_build.stop();
    accumulate_build_timing(timings, cpu_build, wall_build);

    progress.mark_loaded(i + 1);

    const auto [first_index, last_index] = pack_bundle_mesh_range(listing, i);
    (void)first_index;
    if (i == last_index) {
      const std::string& bundle_file = listing.pack_bundles[i];
      reader.release_loaded_bundle();
      progress.on_bundle_extracted(bundle_file, last_index);
    }
  }
}

void process_per_file_meshes(const IndexVerticesConfig& config, const IndexVerticesLabels& labels,
                             const fs::path& input_dir, DatasetMeshListing& listing,
                             TimingTotals& timings, std::vector<KdTreeBundleEntry>& kd_trees,
                             std::vector<RsTreeBundleEntry>& rs_trees) {
  const std::string load_label = std::string(labels.command) + " mesh files";
  listing = list_dataset_meshes_for_command(input_dir, ply_list_options(config.max_objects),
                                              labels.command);

  DatasetMeshLoadProgress progress(listing, load_label);
  const std::size_t mesh_count = listing.paths.size();
  if (config.kind == VertexIndexKind::Kd) {
    kd_trees.reserve(mesh_count);
  } else {
    rs_trees.reserve(mesh_count);
  }

  for (std::size_t i = 0; i < mesh_count; ++i) {
    CpuTimer cpu_read;
    WallTimer wall_read;
    cpu_read.start();
    wall_read.start();
    TinMesh mesh = read_ply(listing.paths[i].string(), PlyReadContent::VerticesOnly);
    cpu_read.stop();
    wall_read.stop();
    accumulate_timing(timings, cpu_read, wall_read);

    require_non_empty_mesh(mesh, labels.command, listing.paths[i]);
    const std::string name = listing.paths[i].stem().string();

    CpuTimer cpu_build;
    WallTimer wall_build;
    cpu_build.start();
    wall_build.start();
    std::vector<Point> points = tin_mesh_vertices(mesh);
    if (config.kind == VertexIndexKind::Kd) {
      build_kd_tree_for_mesh(kd_trees, name, std::move(points));
    } else {
      build_rs_tree_for_mesh(rs_trees, name, std::move(points));
    }
    cpu_build.stop();
    wall_build.stop();
    accumulate_build_timing(timings, cpu_build, wall_build);

    progress.mark_loaded(i + 1);
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

  DatasetMeshListing listing;
  TimingTotals timings;
  std::vector<KdTreeBundleEntry> kd_trees;
  std::vector<RsTreeBundleEntry> rs_trees;

  if (const std::optional<fs::path> manifest = find_pack_manifest_for_dataset(input_dir)) {
    process_pack_meshes(config, labels, input_dir, *manifest, listing, timings, kd_trees,
                        rs_trees);
  } else {
    process_per_file_meshes(config, labels, input_dir, listing, timings, kd_trees, rs_trees);
  }

  CpuTimer cpu_save;
  WallTimer wall_save;
  cpu_save.start();
  wall_save.start();
  if (config.kind == VertexIndexKind::Kd) {
    save_kd_vertex_indexes(config, output_dir, kd_trees);
  } else {
    save_rs_vertex_indexes(config, output_dir, rs_trees);
  }
  cpu_save.stop();
  wall_save.stop();

  const std::size_t mesh_count =
      config.kind == VertexIndexKind::Kd ? kd_trees.size() : rs_trees.size();

  std::cout << labels.command << '\n'
            << "  input: " << input_dir.string() << " (" << mesh_count << " meshes)\n";
  print_dataset_mesh_source(std::cout, listing);
  std::cout << "  output: " << output_dir.string();
  if (config.combined_output) {
    std::cout << " (" << config.combined_file << ")\n";
  } else {
    std::cout << " (" << labels.per_file_label << ")\n";
  }
  print_timing_totals(std::string(labels.command) + " read mesh files", timings);
  print_build_timing_totals(std::string(labels.command) + " build " + labels.build_label, timings);
  print_cpu_wall_timing(std::string(labels.command) + " save " + labels.build_label, cpu_save,
                        wall_save);

  return EXIT_SUCCESS;
}

}  // namespace tin_gen
