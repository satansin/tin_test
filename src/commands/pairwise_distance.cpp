#include "tin_gen/commands/pairwise_distance.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/distance_algorithm.hpp"
#include "tin_gen/kd_tree.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/rs_tree.hpp"
#include "tin_gen/tin_mesh.hpp"
#include "tin_gen/vertex_distance.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

using Point = RsTree3d::Point;

constexpr const char* kCommand = "pairwise_distance";

enum class SpatialIndexKind { Rs, Kd };

struct MeshIndex {
  std::string name;
  std::vector<Point> vertices;
  RsTree3d rs_tree;
  KdTree3d kd_tree;
  SpatialIndexKind index_kind = SpatialIndexKind::Rs;
};

constexpr float kVertexMatchEpsilon = 1e-5f;

void verify_vertices_match_points(const std::vector<Point>& vertices,
                                  const std::vector<Point>& tree_points,
                                  const std::string& context) {
  if (vertices.size() != tree_points.size()) {
    throw std::runtime_error("pairwise_distance: vertex count mismatch for " + context);
  }
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    for (int axis = 0; axis < 3; ++axis) {
      const float ply_coord =
          static_cast<float>(vertices[i][static_cast<std::size_t>(axis)]);
      const float tree_coord =
          static_cast<float>(tree_points[i][static_cast<std::size_t>(axis)]);
      if (std::abs(ply_coord - tree_coord) > kVertexMatchEpsilon) {
        throw std::runtime_error("pairwise_distance: ply vertices do not match index points for " +
                                 context);
      }
    }
  }
}

void verify_vertices_match_rs_tree(const std::vector<Point>& vertices, const RsTree3d& tree,
                                  const std::string& context) {
  verify_rs_tree_vertex_count(vertices.size(), tree, context);
  verify_vertices_match_points(vertices, tree.points(), context);
}

void verify_vertices_match_kdtree(const std::vector<Point>& vertices, const KdTree3d& tree,
                                  const std::string& context) {
  verify_kdtree_vertex_count(vertices.size(), tree, context);
  verify_vertices_match_points(vertices, tree.points(), context);
}

std::vector<std::size_t> expected_vertex_counts(const std::vector<MeshIndex>& meshes) {
  std::vector<std::size_t> counts;
  counts.reserve(meshes.size());
  for (const auto& mesh : meshes) {
    counts.push_back(mesh.vertices.size());
  }
  return counts;
}

void build_in_memory_rs_trees(std::vector<MeshIndex>& meshes) {
  for (auto& entry : meshes) {
    entry.rs_tree = RsTree3d(entry.vertices);
    entry.index_kind = SpatialIndexKind::Rs;
  }
}

void assign_rs_trees_from_folder(std::vector<MeshIndex>& meshes,
                                const std::vector<fs::path>& ply_files,
                                const fs::path& rs_dir, LoadRsTreesFromFolderResult& loaded) {
  if (meshes.size() != ply_files.size()) {
    throw std::logic_error(std::string(kCommand) + ": mesh/path count mismatch");
  }

  loaded = load_rs_trees_from_folder(rs_dir, ply_files, expected_vertex_counts(meshes));

  for (std::size_t i = 0; i < meshes.size(); ++i) {
    const std::string stem = ply_files[i].stem().string();
    auto tree_it = loaded.trees_by_stem.find(stem);
    if (tree_it == loaded.trees_by_stem.end()) {
      throw std::runtime_error("pairwise_distance: rs missing entry for " + stem);
    }
    meshes[i].rs_tree = std::move(tree_it->second);
    meshes[i].index_kind = SpatialIndexKind::Rs;
    const std::string context =
        loaded.source == RsTreeFolderLoadSource::Bundle
            ? loaded.bundle_path->string() + " [" + stem + "]"
            : (rs_dir / (stem + ".rstree")).string();
    verify_vertices_match_rs_tree(meshes[i].vertices, meshes[i].rs_tree, context);
  }
}

void assign_kdtrees_from_folder(std::vector<MeshIndex>& meshes,
                                const std::vector<fs::path>& ply_files,
                                const fs::path& kd_dir, LoadKdTreesFromFolderResult& loaded) {
  if (meshes.size() != ply_files.size()) {
    throw std::logic_error(std::string(kCommand) + ": mesh/path count mismatch");
  }

  loaded = load_kdtrees_from_folder(kd_dir, ply_files, expected_vertex_counts(meshes));

  for (std::size_t i = 0; i < meshes.size(); ++i) {
    const std::string stem = ply_files[i].stem().string();
    auto tree_it = loaded.trees_by_stem.find(stem);
    if (tree_it == loaded.trees_by_stem.end()) {
      throw std::runtime_error("pairwise_distance: kdtree missing entry for " + stem);
    }
    meshes[i].kd_tree = std::move(tree_it->second);
    meshes[i].index_kind = SpatialIndexKind::Kd;
    const std::string context =
        loaded.source == KdTreeFolderLoadSource::Bundle
            ? loaded.bundle_path->string() + " [" + stem + "]"
            : (kd_dir / (stem + ".kdtree")).string();
    verify_vertices_match_kdtree(meshes[i].vertices, meshes[i].kd_tree, context);
  }
}

double vertex_pair_distance(const MeshIndex& a, const MeshIndex& b) {
  if (a.index_kind == SpatialIndexKind::Kd || b.index_kind == SpatialIndexKind::Kd) {
    const VertexDistanceResult result =
        symmetric_vertex_distance(a.vertices, b.kd_tree, b.vertices, a.kd_tree);
    return result.distance;
  }
  const VertexDistanceResult result =
      symmetric_vertex_distance_index(a.vertices, b.rs_tree, b.vertices, a.rs_tree);
  return result.distance;
}

class PairwiseMatrixProgress {
 public:
  explicit PairwiseMatrixProgress(std::size_t num_meshes,
                                  std::string_view label = "pairwise_distance matrix")
      : total_pairs_(total_pair_computations(num_meshes)),
        label_(label),
        start_(std::chrono::steady_clock::now()) {
    interval_ = compute_interval(total_pairs_);
  }

  void mark_pair_completed(std::size_t completed_pairs) {
    if (total_pairs_ == 0 || completed_pairs == 0 || completed_pairs > total_pairs_) {
      return;
    }
    const bool at_interval = completed_pairs % interval_ == 0;
    const bool at_end = completed_pairs == total_pairs_;
    if (!at_interval && !at_end) {
      return;
    }
    const auto now = std::chrono::steady_clock::now();
    const double elapsed =
        std::chrono::duration<double>(now - start_).count();
    report_folder_mesh_load_progress(completed_pairs, total_pairs_, elapsed, label_);
  }

 private:
  static std::size_t total_pair_computations(const std::size_t num_meshes) {
    if (num_meshes <= 1) {
      return 0;
    }
    return num_meshes * (num_meshes - 1) / 2;
  }

  static std::size_t compute_interval(const std::size_t total_pairs) {
    if (total_pairs <= 1) {
      return 1;
    }
    const std::size_t tenth = (total_pairs + 9) / 10;
    return std::min(kFolderMeshLoadProgressInterval, std::max<std::size_t>(1, tenth));
  }

  std::size_t total_pairs_ = 0;
  std::size_t interval_ = 1;
  std::string label_;
  std::chrono::steady_clock::time_point start_;
};

void write_pairwise_matrix_vertex(const std::vector<MeshIndex>& meshes,
                                  const DistanceAlgorithm algorithm,
                                  const fs::path& output_path, SpatialIndexKind index_kind,
                                  const std::optional<fs::path>& index_dir,
                                  const std::optional<fs::path>& index_bundle) {
  const std::size_t n = meshes.size();
  std::vector<std::vector<double>> matrix(n, std::vector<double>(n, 0.0));

  PairwiseMatrixProgress matrix_progress(n);
  std::size_t pairs_done = 0;
  for (std::size_t i = 0; i < n; ++i) {
    matrix[i][i] = 0.0;
    for (std::size_t j = i + 1; j < n; ++j) {
      const double distance = vertex_pair_distance(meshes[i], meshes[j]);
      matrix[i][j] = distance;
      matrix[j][i] = distance;
      matrix_progress.mark_pair_completed(++pairs_done);
    }
  }

  fs::create_directories(output_path.parent_path());

  std::ofstream out(output_path);
  if (!out) {
    throw std::runtime_error("pairwise_distance: cannot write " + output_path.string());
  }

  out << std::setprecision(17);
  out << "# tin_test pairwise_distance\n";
  out << "# algorithm: " << distance_algorithm_name(algorithm) << '\n';
  if (index_kind == SpatialIndexKind::Kd) {
    if (index_dir) {
      out << "# kd_dir: " << index_dir->string() << '\n';
      if (index_bundle) {
        out << "# kdtree_bundle: " << index_bundle->string() << '\n';
      }
    } else {
      out << "# kdtree: in-memory (built from ply vertices)\n";
    }
  } else if (index_dir) {
    out << "# rs_dir: " << index_dir->string() << '\n';
    if (index_bundle) {
      out << "# rs_bundle: " << index_bundle->string() << '\n';
    }
  } else {
    out << "# rs: in-memory (built from ply vertices)\n";
  }
  out << "# n=" << n << "  matrix[i][j] = distance between object i and object j (0-based)\n";
  for (std::size_t i = 0; i < n; ++i) {
    out << "#   " << i << ' ' << meshes[i].name << '\n';
  }
  out << n << '\n';
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      if (j > 0) {
        out << ' ';
      }
      out << matrix[i][j];
    }
    out << '\n';
  }
}

}  // namespace

int run_pairwise_distance(const PairwiseDistanceConfig& config) {
  const fs::path input_dir(config.input_dir);
  const fs::path output_path(config.output_path);

  if (config.kd_dir && config.rs_dir) {
    throw std::runtime_error("pairwise_distance: --kd-dir and --rs-dir are mutually exclusive");
  }

  std::optional<fs::path> rs_dir;
  if (config.rs_dir) {
    rs_dir = fs::path(*config.rs_dir);
    if (!fs::exists(*rs_dir) || !fs::is_directory(*rs_dir)) {
      throw std::runtime_error("pairwise_distance: rs_dir is not a directory: " +
                               rs_dir->string());
    }
  }

  std::optional<fs::path> kd_dir;
  if (config.kd_dir) {
    kd_dir = fs::path(*config.kd_dir);
    if (!fs::exists(*kd_dir) || !fs::is_directory(*kd_dir)) {
      throw std::runtime_error("pairwise_distance: kd_dir is not a directory: " +
                               kd_dir->string());
    }
  }

  if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
    throw std::runtime_error(std::string(kCommand) + ": input_dir is not a directory: " +
                             input_dir.string());
  }

  CpuTimer cpu_read;
  WallTimer wall_read;
  cpu_read.start();
  wall_read.start();
  const LoadedDatasetMeshes loaded_meshes = load_all_dataset_meshes(
      input_dir, ply_list_options(config.max_objects), kCommand, std::string(kCommand) + " mesh files");
  cpu_read.stop();
  wall_read.stop();

  const DatasetMeshListing& listing = loaded_meshes.listing;
  const std::vector<fs::path>& ply_files = listing.paths;

  std::vector<MeshIndex> meshes;
  meshes.reserve(loaded_meshes.meshes.size());
  for (std::size_t i = 0; i < loaded_meshes.meshes.size(); ++i) {
    MeshIndex entry;
    entry.name = ply_files[i].stem().string();
    entry.vertices = tin_mesh_vertices(loaded_meshes.meshes[i]);
    meshes.push_back(std::move(entry));
  }

  SpatialIndexKind index_kind = SpatialIndexKind::Rs;
  std::optional<fs::path> index_dir;
  std::optional<fs::path> index_bundle;

  LoadRsTreesFromFolderResult loaded_rs;
  LoadKdTreesFromFolderResult loaded_kdtrees;

  if (kd_dir) {
    index_kind = SpatialIndexKind::Kd;
    index_dir = kd_dir;
    CpuTimer cpu_load;
    WallTimer wall_load;
    cpu_load.start();
    wall_load.start();
    assign_kdtrees_from_folder(meshes, ply_files, *kd_dir, loaded_kdtrees);
    cpu_load.stop();
    wall_load.stop();
    index_bundle = loaded_kdtrees.bundle_path;
    print_cpu_wall_timing("pairwise_distance kdtree load", cpu_load, wall_load);
  } else if (rs_dir) {
    index_kind = SpatialIndexKind::Rs;
    index_dir = rs_dir;
    CpuTimer cpu_load;
    WallTimer wall_load;
    cpu_load.start();
    wall_load.start();
    assign_rs_trees_from_folder(meshes, ply_files, *rs_dir, loaded_rs);
    cpu_load.stop();
    wall_load.stop();
    index_bundle = loaded_rs.bundle_path;
    print_cpu_wall_timing("pairwise_distance rs load", cpu_load, wall_load);
  } else {
    CpuTimer cpu_build;
    WallTimer wall_build;
    cpu_build.start();
    wall_build.start();
    build_in_memory_rs_trees(meshes);
    cpu_build.stop();
    wall_build.stop();
    print_cpu_wall_timing("pairwise_distance r*-tree build", cpu_build, wall_build);
  }

  CpuTimer cpu_matrix;
  WallTimer wall_matrix;
  cpu_matrix.start();
  wall_matrix.start();

  switch (config.algorithm) {
    case DistanceAlgorithm::Vertex:
      write_pairwise_matrix_vertex(meshes, config.algorithm, output_path, index_kind, index_dir,
                                   index_bundle);
      break;
    default:
      throw std::logic_error(std::string(kCommand) + ": unhandled algorithm.");
  }

  cpu_matrix.stop();
  wall_matrix.stop();

  std::cout << kCommand << '\n'
            << "  input: " << input_dir.string() << " (" << meshes.size() << " meshes)\n";
  print_dataset_mesh_source(std::cout, listing);
  std::cout << "  algorithm: " << distance_algorithm_name(config.algorithm) << '\n';
  if (kd_dir) {
    std::cout << "  kd_dir: " << kd_dir->string() << '\n';
    if (loaded_kdtrees.source == KdTreeFolderLoadSource::Bundle &&
        loaded_kdtrees.bundle_path) {
      std::cout << "  kdtree_source: bundle\n";
    } else {
      std::cout << "  kdtree_source: per-file\n";
    }
  } else if (rs_dir) {
    std::cout << "  rs_dir: " << rs_dir->string() << '\n';
    if (loaded_rs.source == RsTreeFolderLoadSource::Bundle &&
        loaded_rs.bundle_path) {
      std::cout << "  rs_source: bundle\n";
    } else {
      std::cout << "  rs_source: per-file\n";
    }
  } else {
    std::cout << "  rs: in-memory (built before distance matrix)\n";
  }
  std::cout << "  output: " << output_path.string() << '\n';
  print_cpu_wall_timing(std::string(kCommand) + " read mesh files", cpu_read, wall_read);
  print_cpu_wall_timing(std::string(kCommand) + " matrix", cpu_matrix, wall_matrix);

  return EXIT_SUCCESS;
}

}  // namespace tin_gen
