#include "tin_gen/commands/pairwise_distance.hpp"

#include "tin_gen/cpu_timer.hpp"
#include "tin_gen/distance_algorithm.hpp"
#include "tin_gen/kd_tree.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/tin_mesh.hpp"
#include "tin_gen/vertex_distance.hpp"

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

struct MeshIndex {
  std::string name;
  std::vector<KdTree3d::Point> vertices;
  KdTree3d tree;
};

constexpr float kVertexMatchEpsilon = 1e-5f;

void verify_vertices_match_kdtree(const std::vector<KdTree3d::Point>& vertices,
                                  const KdTree3d& tree, const std::string& context) {
  verify_kdtree_vertex_count(vertices.size(), tree, context);
  const std::vector<KdTree3d::Point>& tree_points = tree.points();
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    for (int axis = 0; axis < 3; ++axis) {
      const float ply_coord =
          static_cast<float>(vertices[i][static_cast<std::size_t>(axis)]);
      const float tree_coord =
          static_cast<float>(tree_points[i][static_cast<std::size_t>(axis)]);
      if (std::abs(ply_coord - tree_coord) > kVertexMatchEpsilon) {
        throw std::runtime_error("pairwise_distance: ply vertices do not match kdtree points for " +
                                 context);
      }
    }
  }
}

MeshIndex load_mesh_vertices(const fs::path& ply_path) {
  const TinMesh mesh = read_ply(ply_path.string());
  if (mesh.vertices.empty()) {
    throw std::runtime_error("pairwise_distance: mesh has no vertices: " + ply_path.string());
  }

  MeshIndex entry;
  entry.name = ply_path.filename().string();
  entry.vertices.reserve(mesh.vertices.size());
  for (const auto& v : mesh.vertices) {
    entry.vertices.push_back(v);
  }
  return entry;
}

void build_in_memory_kdtrees(std::vector<MeshIndex>& meshes) {
  for (auto& entry : meshes) {
    entry.tree = KdTree3d(entry.vertices);
  }
}

void assign_kdtrees_from_folder(std::vector<MeshIndex>& meshes,
                                const std::vector<fs::path>& ply_files,
                                const fs::path& kdtree_dir, LoadKdTreesFromFolderResult& loaded) {
  if (meshes.size() != ply_files.size()) {
    throw std::logic_error("pairwise_distance: mesh/path count mismatch");
  }

  std::vector<std::size_t> expected_vertex_counts;
  expected_vertex_counts.reserve(meshes.size());
  for (const auto& mesh : meshes) {
    expected_vertex_counts.push_back(mesh.vertices.size());
  }

  loaded = load_kdtrees_from_folder(kdtree_dir, ply_files, expected_vertex_counts);

  for (std::size_t i = 0; i < meshes.size(); ++i) {
    const std::string stem = ply_files[i].stem().string();
    auto tree_it = loaded.trees_by_stem.find(stem);
    if (tree_it == loaded.trees_by_stem.end()) {
      throw std::runtime_error("pairwise_distance: kdtree missing entry for " + stem);
    }
    meshes[i].tree = std::move(tree_it->second);
    const std::string context =
        loaded.source == KdTreeFolderLoadSource::Bundle
            ? loaded.bundle_path->string() + " [" + stem + "]"
            : (kdtree_dir / (stem + ".kdtree")).string();
    verify_vertices_match_kdtree(meshes[i].vertices, meshes[i].tree, context);
  }
}

double vertex_pair_distance(const MeshIndex& a, const MeshIndex& b) {
  const VertexDistanceResult result =
      symmetric_vertex_distance(a.vertices, b.tree, b.vertices, a.tree);
  return result.distance;
}

void write_pairwise_matrix_vertex(const std::vector<MeshIndex>& meshes,
                                  const DistanceAlgorithm algorithm,
                                  const fs::path& output_path,
                                  const std::optional<fs::path>& kdtree_dir,
                                  const std::optional<fs::path>& kdtree_bundle) {
  const std::size_t n = meshes.size();
  std::vector<std::vector<double>> matrix(n, std::vector<double>(n, 0.0));

  for (std::size_t i = 0; i < n; ++i) {
    matrix[i][i] = 0.0;
    for (std::size_t j = i + 1; j < n; ++j) {
      const double distance = vertex_pair_distance(meshes[i], meshes[j]);
      matrix[i][j] = distance;
      matrix[j][i] = distance;
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
  if (kdtree_dir) {
    out << "# kd_dir: " << kdtree_dir->string() << '\n';
    if (kdtree_bundle) {
      out << "# kdtree_bundle: " << kdtree_bundle->string() << '\n';
    }
  } else {
    out << "# kdtree: in-memory (built from ply vertices)\n";
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
  std::optional<fs::path> kdtree_dir;
  if (config.kdtree_dir) {
    kdtree_dir = fs::path(*config.kdtree_dir);
    if (!fs::exists(*kdtree_dir) || !fs::is_directory(*kdtree_dir)) {
      throw std::runtime_error("pairwise_distance: kdtree_dir is not a directory: " +
                               kdtree_dir->string());
    }
  }

  if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
    throw std::runtime_error("pairwise_distance: input_dir is not a directory: " +
                             input_dir.string());
  }

  ListPlyFilesOptions list_opts;
  list_opts.max_objects = config.max_objects;

  std::vector<fs::path> ply_files;
  try {
    ply_files = list_ply_files_in_directory(input_dir, list_opts);
  } catch (const std::runtime_error& error) {
    throw std::runtime_error(std::string("pairwise_distance: ") + error.what());
  }

  std::vector<MeshIndex> meshes;
  meshes.reserve(ply_files.size());
  for (const auto& ply_path : ply_files) {
    meshes.push_back(load_mesh_vertices(ply_path));
  }

  LoadKdTreesFromFolderResult loaded_kdtrees;
  if (kdtree_dir) {
    CpuTimer cpu_load;
    WallTimer wall_load;
    cpu_load.start();
    wall_load.start();
    assign_kdtrees_from_folder(meshes, ply_files, *kdtree_dir, loaded_kdtrees);
    cpu_load.stop();
    wall_load.stop();
    print_cpu_wall_timing("pairwise_distance kdtree load", cpu_load, wall_load);
  } else {
    CpuTimer cpu_build;
    WallTimer wall_build;
    cpu_build.start();
    wall_build.start();
    build_in_memory_kdtrees(meshes);
    cpu_build.stop();
    wall_build.stop();
    print_cpu_wall_timing("pairwise_distance kd-tree build", cpu_build, wall_build);
  }

  CpuTimer cpu_matrix;
  WallTimer wall_matrix;
  cpu_matrix.start();
  wall_matrix.start();

  switch (config.algorithm) {
    case DistanceAlgorithm::Vertex:
      write_pairwise_matrix_vertex(meshes, config.algorithm, output_path, kdtree_dir,
                                   loaded_kdtrees.bundle_path);
      break;
    default:
      throw std::logic_error("Unhandled distance algorithm.");
  }

  cpu_matrix.stop();
  wall_matrix.stop();

  std::cout << "pairwise_distance\n"
            << "  input: " << input_dir.string() << " (" << meshes.size() << " meshes)\n"
            << "  algorithm: " << distance_algorithm_name(config.algorithm) << '\n';
  if (kdtree_dir) {
    std::cout << "  kd_dir: " << kdtree_dir->string() << '\n';
    if (loaded_kdtrees.source == KdTreeFolderLoadSource::Bundle &&
        loaded_kdtrees.bundle_path) {
      std::cout << "  kdtree_source: bundle\n";
    } else {
      std::cout << "  kdtree_source: per-file\n";
    }
  } else {
    std::cout << "  kdtree: in-memory (built before distance matrix)\n";
  }
  std::cout << "  output: " << output_path.string() << '\n';
  print_cpu_wall_timing("pairwise_distance matrix", cpu_matrix, wall_matrix);

  return EXIT_SUCCESS;
}

}  // namespace tin_gen
