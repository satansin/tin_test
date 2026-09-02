#include "tin_gen/commands/topk.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/distance_algorithm.hpp"
#include "tin_gen/kd_tree.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/rs_tree.hpp"
#include "tin_gen/tin_mesh.hpp"
#include "tin_gen/vertex_distance.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

using Point = RsTree3d::Point;

constexpr const char* kCommand = "topk";

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
    throw std::runtime_error(std::string(kCommand) + ": vertex count mismatch for " + context);
  }
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    for (int axis = 0; axis < 3; ++axis) {
      const float ply_coord =
          static_cast<float>(vertices[i][static_cast<std::size_t>(axis)]);
      const float tree_coord =
          static_cast<float>(tree_points[i][static_cast<std::size_t>(axis)]);
      if (std::abs(ply_coord - tree_coord) > kVertexMatchEpsilon) {
        throw std::runtime_error(std::string(kCommand) +
                                 ": ply vertices do not match index points for " + context);
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
                                 const std::vector<fs::path>& ply_files, const fs::path& rs_dir,
                                 LoadRsTreesFromFolderResult& loaded) {
  if (meshes.size() != ply_files.size()) {
    throw std::logic_error(std::string(kCommand) + ": mesh/path count mismatch");
  }

  loaded = load_rs_trees_from_folder(rs_dir, ply_files, expected_vertex_counts(meshes));

  for (std::size_t i = 0; i < meshes.size(); ++i) {
    const std::string stem = ply_files[i].stem().string();
    auto tree_it = loaded.trees_by_stem.find(stem);
    if (tree_it == loaded.trees_by_stem.end()) {
      throw std::runtime_error(std::string(kCommand) + ": rs missing entry for " + stem);
    }
    meshes[i].rs_tree = std::move(tree_it->second);
    meshes[i].index_kind = SpatialIndexKind::Rs;
    const std::string context =
        loaded.source == RsTreeFolderLoadSource::Bundle
            ? loaded.manifest_path->string() + " [" + stem + "]"
            : (rs_dir / (stem + ".rstree")).string();
    verify_vertices_match_rs_tree(meshes[i].vertices, meshes[i].rs_tree, context);
  }
}

void assign_kdtrees_from_folder(std::vector<MeshIndex>& meshes,
                                const std::vector<fs::path>& ply_files, const fs::path& kd_dir,
                                LoadKdTreesFromFolderResult& loaded) {
  if (meshes.size() != ply_files.size()) {
    throw std::logic_error(std::string(kCommand) + ": mesh/path count mismatch");
  }

  loaded = load_kdtrees_from_folder(kd_dir, ply_files, expected_vertex_counts(meshes));

  for (std::size_t i = 0; i < meshes.size(); ++i) {
    const std::string stem = ply_files[i].stem().string();
    auto tree_it = loaded.trees_by_stem.find(stem);
    if (tree_it == loaded.trees_by_stem.end()) {
      throw std::runtime_error(std::string(kCommand) + ": kdtree missing entry for " + stem);
    }
    meshes[i].kd_tree = std::move(tree_it->second);
    meshes[i].index_kind = SpatialIndexKind::Kd;
    const std::string context =
        loaded.source == KdTreeFolderLoadSource::Bundle
            ? loaded.manifest_path->string() + " [" + stem + "]"
            : (kd_dir / (stem + ".kdtree")).string();
    verify_vertices_match_kdtree(meshes[i].vertices, meshes[i].kd_tree, context);
  }
}

double pair_distance(const MeshIndex& a, const MeshIndex& b, const DistanceAlgorithm algorithm) {
  const bool use_kd =
      a.index_kind == SpatialIndexKind::Kd || b.index_kind == SpatialIndexKind::Kd;
  switch (algorithm) {
    case DistanceAlgorithm::Vertex: {
      const VertexDistanceResult result =
          use_kd ? symmetric_vertex_distance(a.vertices, b.kd_tree, b.vertices, a.kd_tree)
                 : symmetric_vertex_distance_index(a.vertices, b.rs_tree, b.vertices, a.rs_tree);
      return result.distance;
    }
    case DistanceAlgorithm::Chamfer: {
      const VertexDistanceResult result =
          use_kd ? symmetric_chamfer_distance(a.vertices, b.kd_tree, b.vertices, a.kd_tree)
                 : symmetric_chamfer_distance_index(a.vertices, b.rs_tree, b.vertices, a.rs_tree);
      return result.distance;
    }
    case DistanceAlgorithm::Hausdorff: {
      const VertexDistanceResult result =
          use_kd ? symmetric_hausdorff_distance(a.vertices, b.kd_tree, b.vertices, a.kd_tree)
                 : symmetric_hausdorff_distance_index(a.vertices, b.rs_tree, b.vertices, a.rs_tree);
      return result.distance;
    }
  }
  throw std::logic_error(std::string(kCommand) + ": unhandled algorithm.");
}

struct RankedHit {
  std::string name;
  double distance = 0.0;
};

}  // namespace

int run_topk(const TopKConfig& config) {
  const fs::path input_dir(config.input_dir);
  const fs::path query_path(config.query_path);

  if (config.kd_dir && config.rs_dir) {
    throw std::runtime_error(std::string(kCommand) +
                             ": --kd-dir and --rs-dir are mutually exclusive");
  }
  if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
    throw std::runtime_error(std::string(kCommand) + ": input_dir is not a directory: " +
                             input_dir.string());
  }
  if (!fs::exists(query_path) || !fs::is_regular_file(query_path)) {
    throw std::runtime_error(std::string(kCommand) + ": query is not a file: " +
                             query_path.string());
  }

  std::optional<fs::path> rs_dir;
  if (config.rs_dir) {
    rs_dir = fs::path(*config.rs_dir);
    if (!fs::exists(*rs_dir) || !fs::is_directory(*rs_dir)) {
      throw std::runtime_error(std::string(kCommand) + ": rs_dir is not a directory: " +
                               rs_dir->string());
    }
  }

  std::optional<fs::path> kd_dir;
  if (config.kd_dir) {
    kd_dir = fs::path(*config.kd_dir);
    if (!fs::exists(*kd_dir) || !fs::is_directory(*kd_dir)) {
      throw std::runtime_error(std::string(kCommand) + ": kd_dir is not a directory: " +
                               kd_dir->string());
    }
  }

  CpuTimer cpu_read;
  WallTimer wall_read;
  cpu_read.start();
  wall_read.start();

  const TinMesh query_mesh = read_ply(query_path.string(), PlyReadContent::VerticesOnly);
  MeshIndex query;
  query.name = query_path.stem().string();
  query.vertices = tin_mesh_vertices(query_mesh);

  const LoadedDatasetMeshes loaded_meshes =
      load_all_dataset_meshes(input_dir, ply_list_options(config.max_objects), kCommand,
                              std::string(kCommand) + " mesh files", PlyReadContent::VerticesOnly);
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

  LoadRsTreesFromFolderResult loaded_rs;
  LoadKdTreesFromFolderResult loaded_kdtrees;

  if (kd_dir) {
    CpuTimer cpu_load;
    WallTimer wall_load;
    cpu_load.start();
    wall_load.start();
    assign_kdtrees_from_folder(meshes, ply_files, *kd_dir, loaded_kdtrees);
    query.kd_tree = KdTree3d(query.vertices);
    query.index_kind = SpatialIndexKind::Kd;
    cpu_load.stop();
    wall_load.stop();
    print_cpu_wall_timing(std::string(kCommand) + " kdtree load", cpu_load, wall_load);
  } else if (rs_dir) {
    CpuTimer cpu_load;
    WallTimer wall_load;
    cpu_load.start();
    wall_load.start();
    assign_rs_trees_from_folder(meshes, ply_files, *rs_dir, loaded_rs);
    query.rs_tree = RsTree3d(query.vertices);
    query.index_kind = SpatialIndexKind::Rs;
    cpu_load.stop();
    wall_load.stop();
    print_cpu_wall_timing(std::string(kCommand) + " rs load", cpu_load, wall_load);
  } else {
    CpuTimer cpu_build;
    WallTimer wall_build;
    cpu_build.start();
    wall_build.start();
    build_in_memory_rs_trees(meshes);
    query.rs_tree = RsTree3d(query.vertices);
    query.index_kind = SpatialIndexKind::Rs;
    cpu_build.stop();
    wall_build.stop();
    print_cpu_wall_timing(std::string(kCommand) + " r*-tree build", cpu_build, wall_build);
  }

  CpuTimer cpu_scan;
  WallTimer wall_scan;
  cpu_scan.start();
  wall_scan.start();

  std::vector<RankedHit> hits;
  hits.reserve(meshes.size());
  for (const auto& mesh : meshes) {
    if (!config.include_self && mesh.name == query.name) {
      continue;
    }
    RankedHit hit;
    hit.name = mesh.name;
    hit.distance = pair_distance(query, mesh, config.algorithm);
    hits.push_back(std::move(hit));
  }

  if (hits.empty()) {
    throw std::runtime_error(std::string(kCommand) + ": no candidates after filtering");
  }

  const std::size_t num_candidates = hits.size();
  const std::size_t top_k = std::min(config.k, hits.size());
  std::partial_sort(hits.begin(), hits.begin() + static_cast<std::ptrdiff_t>(top_k), hits.end(),
                    [](const RankedHit& a, const RankedHit& b) {
                      if (a.distance != b.distance) {
                        return a.distance < b.distance;
                      }
                      return a.name < b.name;
                    });
  hits.resize(top_k);

  cpu_scan.stop();
  wall_scan.stop();

  std::cout << kCommand << '\n'
            << "  input: " << input_dir.string() << " (" << meshes.size() << " meshes)\n";
  print_dataset_mesh_source(std::cout, listing);
  std::cout << "  query: " << query_path.string() << '\n'
            << "  algorithm: " << distance_algorithm_name(config.algorithm) << '\n'
            << "  k: " << top_k << '\n'
            << "  candidates: " << num_candidates << '\n';
  if (kd_dir) {
    std::cout << "  kd_dir: " << kd_dir->string() << '\n';
  } else if (rs_dir) {
    std::cout << "  rs_dir: " << rs_dir->string() << '\n';
  } else {
    std::cout << "  rs: in-memory (built before scan)\n";
  }
  std::cout << "  include_self: " << (config.include_self ? "true" : "false") << '\n';

  std::cout << "  rank\tname\tdistance\n";
  for (std::size_t i = 0; i < hits.size(); ++i) {
    std::cout << "  " << (i + 1) << '\t' << hits[i].name << '\t' << std::setprecision(17)
              << hits[i].distance << '\n';
  }

  print_cpu_wall_timing(std::string(kCommand) + " read", cpu_read, wall_read);
  print_cpu_wall_timing(std::string(kCommand) + " scan", cpu_scan, wall_scan);

  if (!config.output_path.empty()) {
    const fs::path output_path(config.output_path);
    if (output_path.has_parent_path()) {
      fs::create_directories(output_path.parent_path());
    }
    std::ofstream out(output_path);
    if (!out) {
      throw std::runtime_error(std::string(kCommand) + ": cannot write " + output_path.string());
    }
    out << std::setprecision(17);
    out << "# tin_test topk\n";
    out << "# algorithm: " << distance_algorithm_name(config.algorithm) << '\n';
    out << "# query: " << query_path.string() << '\n';
    out << "# query_stem: " << query.name << '\n';
    out << "# k: " << top_k << '\n';
    out << "# dataset_meshes: " << meshes.size() << '\n';
    out << "# candidates: " << num_candidates << '\n';
    out << "# include_self: " << (config.include_self ? "true" : "false") << '\n';
    out << "# rank name distance\n";
    for (std::size_t i = 0; i < hits.size(); ++i) {
      out << (i + 1) << ' ' << hits[i].name << ' ' << hits[i].distance << '\n';
    }
    std::cout << "  output: " << output_path.string() << '\n';
  }

  return EXIT_SUCCESS;
}

}  // namespace tin_gen
