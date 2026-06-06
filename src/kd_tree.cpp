#include "tin_gen/kd_tree.hpp"

#include "tin_gen/index_merge.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

constexpr char kMagic[8] = {'T', 'I', 'N', 'K', 'D', 'V', '1', '\0'};
constexpr char kBundleMagic[8] = {'T', 'I', 'N', 'K', 'D', 'B', '1', '\0'};

void write_u64(std::ostream& out, const std::uint64_t v) {
  out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

void write_i32(std::ostream& out, const std::int32_t v) {
  out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

void write_f32(std::ostream& out, const float v) {
  out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

std::uint64_t read_u64(std::istream& in) {
  std::uint64_t v = 0;
  in.read(reinterpret_cast<char*>(&v), sizeof(v));
  if (!in) {
    throw std::runtime_error("Unexpected EOF reading kd-tree file");
  }
  return v;
}

std::int32_t read_i32(std::istream& in) {
  std::int32_t v = 0;
  in.read(reinterpret_cast<char*>(&v), sizeof(v));
  if (!in) {
    throw std::runtime_error("Unexpected EOF reading kd-tree file");
  }
  return v;
}

float read_f32(std::istream& in) {
  float v = 0.0f;
  in.read(reinterpret_cast<char*>(&v), sizeof(v));
  if (!in) {
    throw std::runtime_error("Unexpected EOF reading kd-tree file");
  }
  return v;
}

double variance_axis(const std::vector<KdTree3d::Point>& points,
                     const std::vector<std::size_t>& indices, const int axis) {
  if (indices.empty()) {
    return 0.0;
  }
  double mean = 0.0;
  for (const std::size_t i : indices) {
    mean += points[i][static_cast<std::size_t>(axis)];
  }
  mean /= static_cast<double>(indices.size());
  double var = 0.0;
  for (const std::size_t i : indices) {
    const double d = points[i][static_cast<std::size_t>(axis)] - mean;
    var += d * d;
  }
  return var;
}

}  // namespace

KdTree3d::KdTree3d(std::vector<Point> points) : points_(std::move(points)) {
  if (points_.empty()) {
    throw std::runtime_error("Cannot build KD-tree from 0 points");
  }
  std::vector<std::size_t> indices(points_.size());
  for (std::size_t i = 0; i < indices.size(); ++i) {
    indices[i] = i;
  }
  nodes_.reserve(points_.size() * 2);
  build_recursive(indices, 0);
}

int KdTree3d::build_recursive(std::vector<std::size_t>& indices, const int depth) {
  const int node_index = static_cast<int>(nodes_.size());
  nodes_.push_back({});

  if (indices.size() == 1) {
    nodes_[static_cast<std::size_t>(node_index)].axis = -1;
    nodes_[static_cast<std::size_t>(node_index)].point_index = indices.front();
    return node_index;
  }

  int best_axis = depth % 3;
  double best_var = -1.0;
  for (int axis = 0; axis < 3; ++axis) {
    const double v = variance_axis(points_, indices, axis);
    if (v > best_var) {
      best_var = v;
      best_axis = axis;
    }
  }

  const auto cmp = [&](const std::size_t a, const std::size_t b) {
    return points_[a][static_cast<std::size_t>(best_axis)] <
           points_[b][static_cast<std::size_t>(best_axis)];
  };
  const std::size_t mid = indices.size() / 2;
  std::nth_element(indices.begin(), indices.begin() + static_cast<std::ptrdiff_t>(mid),
                   indices.end(), cmp);

  const std::size_t split_index = indices[mid];
  const float split_value = static_cast<float>(points_[split_index][best_axis]);

  std::vector<std::size_t> left(indices.begin(), indices.begin() + static_cast<std::ptrdiff_t>(mid));
  std::vector<std::size_t> right(indices.begin() + static_cast<std::ptrdiff_t>(mid), indices.end());

  nodes_[static_cast<std::size_t>(node_index)].axis = best_axis;
  nodes_[static_cast<std::size_t>(node_index)].split = split_value;
  nodes_[static_cast<std::size_t>(node_index)].left = build_recursive(left, depth + 1);
  nodes_[static_cast<std::size_t>(node_index)].right = build_recursive(right, depth + 1);
  return node_index;
}

void KdTree3d::write_body(std::ostream& out) const {
  write_u64(out, static_cast<std::uint64_t>(points_.size()));
  for (const auto& p : points_) {
    const float xyz[3] = {static_cast<float>(p[0]), static_cast<float>(p[1]),
                          static_cast<float>(p[2])};
    out.write(reinterpret_cast<const char*>(xyz), sizeof(xyz));
  }

  write_u64(out, static_cast<std::uint64_t>(nodes_.size()));
  for (const auto& node : nodes_) {
    write_i32(out, static_cast<std::int32_t>(node.axis));
    write_u64(out, static_cast<std::uint64_t>(node.point_index));
    write_f32(out, node.split);
    write_i32(out, node.left);
    write_i32(out, node.right);
  }
}

void KdTree3d::read_body(std::istream& in) {
  points_.resize(static_cast<std::size_t>(read_u64(in)));
  for (auto& p : points_) {
    float xyz[3];
    in.read(reinterpret_cast<char*>(xyz), sizeof(xyz));
    if (!in) {
      throw std::runtime_error("Unexpected EOF reading kd-tree vertices");
    }
    p = {xyz[0], xyz[1], xyz[2]};
  }

  nodes_.resize(static_cast<std::size_t>(read_u64(in)));
  for (auto& node : nodes_) {
    node.axis = read_i32(in);
    node.point_index = static_cast<std::size_t>(read_u64(in));
    node.split = read_f32(in);
    node.left = read_i32(in);
    node.right = read_i32(in);
  }

  if (points_.empty() || nodes_.empty()) {
    throw std::runtime_error("Invalid empty kd-tree data");
  }
}

void KdTree3d::save(const std::string& filepath) const {
  std::ofstream out(filepath, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Failed to open KD-tree file for writing: " + filepath);
  }
  out.write(kMagic, 8);
  write_body(out);
}

KdTree3d KdTree3d::load(const std::string& filepath) {
  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open KD-tree file for reading: " + filepath);
  }

  char magic[8]{};
  in.read(magic, 8);
  if (std::string(magic, 8) != std::string(kMagic, 8)) {
    throw std::runtime_error("Not a TINKDV1 kd-tree file: " + filepath);
  }

  KdTree3d tree;
  tree.read_body(in);
  return tree;
}

std::vector<std::uint8_t> KdTree3d::serialize() const {
  std::ostringstream out(std::ios::binary);
  out.write(kMagic, 8);
  write_body(out);
  const std::string data = out.str();
  return std::vector<std::uint8_t>(data.begin(), data.end());
}

KdTree3d KdTree3d::deserialize(const std::vector<std::uint8_t>& bytes) {
  if (bytes.size() < 8) {
    throw std::runtime_error("KD-tree buffer too small");
  }
  if (std::memcmp(bytes.data(), kMagic, 8) != 0) {
    throw std::runtime_error("Not a TINKDV1 kd-tree buffer");
  }
  std::istringstream in(std::string(bytes.begin() + 8, bytes.end()), std::ios::binary);
  KdTree3d tree;
  tree.read_body(in);
  return tree;
}

namespace {

struct BundleTocEntry {
  std::string name;
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
};

void read_bundle_magic(std::istream& in, const std::string& filepath) {
  char magic[8]{};
  in.read(magic, 8);
  if (std::string(magic, 8) != std::string(kBundleMagic, 8)) {
    throw std::runtime_error("Not a TINKDB1 kd-tree bundle: " + filepath);
  }
}

std::vector<BundleTocEntry> read_bundle_toc(std::istream& in, const std::string& filepath) {
  read_bundle_magic(in, filepath);
  const std::uint64_t count = read_u64(in);
  std::vector<BundleTocEntry> toc(static_cast<std::size_t>(count));
  for (auto& entry : toc) {
    const std::uint64_t name_len = read_u64(in);
    entry.name.resize(static_cast<std::size_t>(name_len));
    in.read(entry.name.data(), static_cast<std::streamsize>(name_len));
    if (!in) {
      throw std::runtime_error("Unexpected EOF reading bundle entry name: " + filepath);
    }
    entry.offset = read_u64(in);
    entry.size = read_u64(in);
  }
  return toc;
}

KdTree3d load_tree_at_offset(const std::string& filepath, const std::uint64_t offset,
                             const std::uint64_t size) {
  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open kd-tree bundle: " + filepath);
  }
  in.seekg(static_cast<std::streamoff>(offset));
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
  if (!in) {
    throw std::runtime_error("Unexpected EOF reading kd-tree bundle blob: " + filepath);
  }
  return KdTree3d::deserialize(bytes);
}

}  // namespace

void save_kd_tree_bundle(const std::string& filepath,
                         const std::vector<KdTreeBundleEntry>& entries) {
  if (entries.empty()) {
    throw std::runtime_error("Cannot write empty kd-tree bundle");
  }

  std::vector<std::vector<std::uint8_t>> blobs;
  blobs.reserve(entries.size());
  for (const auto& entry : entries) {
    blobs.push_back(entry.tree.serialize());
  }

  std::vector<BundleTocEntry> toc;
  toc.reserve(entries.size());
  for (std::size_t i = 0; i < entries.size(); ++i) {
    BundleTocEntry row;
    row.name = entries[i].name;
    row.size = static_cast<std::uint64_t>(blobs[i].size());
    toc.push_back(std::move(row));
  }

  std::uint64_t offset = 8 + 8;
  for (const auto& row : toc) {
    offset += 8 + static_cast<std::uint64_t>(row.name.size()) + 8 + 8;
  }
  for (std::size_t i = 0; i < toc.size(); ++i) {
    toc[i].offset = offset;
    offset += toc[i].size;
  }

  std::ofstream out(filepath, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Failed to open kd-tree bundle for writing: " + filepath);
  }

  out.write(kBundleMagic, 8);
  write_u64(out, static_cast<std::uint64_t>(entries.size()));
  for (std::size_t i = 0; i < entries.size(); ++i) {
    write_u64(out, static_cast<std::uint64_t>(toc[i].name.size()));
    out.write(toc[i].name.data(), static_cast<std::streamsize>(toc[i].name.size()));
    write_u64(out, toc[i].offset);
    write_u64(out, toc[i].size);
  }
  for (const auto& blob : blobs) {
    out.write(reinterpret_cast<const char*>(blob.data()),
              static_cast<std::streamsize>(blob.size()));
  }
}

std::vector<KdTreeBundleEntry> load_kd_tree_bundle(const std::string& filepath) {
  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open kd-tree bundle: " + filepath);
  }
  const std::vector<BundleTocEntry> toc = read_bundle_toc(in, filepath);

  std::vector<KdTreeBundleEntry> result;
  result.reserve(toc.size());
  for (const auto& row : toc) {
    result.push_back({row.name, load_tree_at_offset(filepath, row.offset, row.size)});
  }
  return result;
}

bool is_kdtree_bundle_file(const fs::path& path) {
  if (!fs::exists(path) || !fs::is_regular_file(path)) {
    return false;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return false;
  }
  char magic[8]{};
  in.read(magic, 8);
  if (!in) {
    return false;
  }
  return std::string(magic, 8) == std::string(kBundleMagic, 8);
}

void verify_kdtree_vertex_count(const std::size_t expected_vertex_count, const KdTree3d& tree,
                                const std::string& context) {
  if (tree.points().size() != expected_vertex_count) {
    throw std::runtime_error("kdtree vertex count mismatch for " + context + " (expected=" +
                           std::to_string(expected_vertex_count) + ", kdtree=" +
                           std::to_string(tree.points().size()) + ")");
  }
}

LoadKdTreesFromFolderResult load_kdtrees_from_folder(
    const fs::path& kdtree_dir, const std::vector<fs::path>& ply_files,
    const std::vector<std::size_t>& expected_vertex_counts) {
  if (expected_vertex_counts.size() != ply_files.size()) {
    throw std::invalid_argument("load_kdtrees_from_folder: expected_vertex_counts size must match "
                                "ply_files");
  }

  LoadKdTreesFromFolderResult result;

  if (const std::optional<fs::path> manifest_path = find_kd_index_merge_manifest(kdtree_dir)) {
    const IndexMergeManifest manifest = load_kd_index_merge_manifest(*manifest_path);
    result.source = KdTreeFolderLoadSource::Bundle;
    result.manifest_path = manifest_path;

    std::unordered_set<std::string> loaded_bundles;
    for (const IndexMergeEntry& entry : manifest.entries) {
      if (loaded_bundles.insert(entry.bundle_file).second) {
        const fs::path bundle_path = kdtree_dir / entry.bundle_file;
        const std::vector<KdTreeBundleEntry> entries = load_kd_tree_bundle(bundle_path.string());
        for (auto& bundle_entry : entries) {
          result.trees_by_stem.emplace(std::move(bundle_entry.name), std::move(bundle_entry.tree));
        }
      }
    }
  } else {
    result.source = KdTreeFolderLoadSource::PerFile;
    for (const auto& ply_path : ply_files) {
      const std::string stem = ply_path.stem().string();
      const fs::path kdtree_path = kdtree_dir / (stem + ".kdtree");
      if (!fs::exists(kdtree_path)) {
        throw std::runtime_error("kdtree file not found: " + kdtree_path.string());
      }
      if (is_kdtree_bundle_file(kdtree_path)) {
        throw std::runtime_error(
            "expected per-mesh kdtree file but found bundle (use kd_merge_manifest.txt): " +
            kdtree_path.string());
      }
      result.trees_by_stem.emplace(stem, KdTree3d::load(kdtree_path.string()));
    }
  }

  for (std::size_t i = 0; i < ply_files.size(); ++i) {
    const std::string stem = ply_files[i].stem().string();
    const auto tree_it = result.trees_by_stem.find(stem);
    if (tree_it == result.trees_by_stem.end()) {
      throw std::runtime_error("kdtree missing entry for mesh stem: " + stem);
    }
    const std::string context =
        result.source == KdTreeFolderLoadSource::Bundle
            ? result.manifest_path->string() + " [" + stem + "]"
            : (kdtree_dir / (stem + ".kdtree")).string();
    verify_kdtree_vertex_count(expected_vertex_counts[i], tree_it->second, context);
  }

  return result;
}

std::vector<KdTreeBundleEntry> load_kd_tree_bundle_last(const std::string& filepath,
                                                         const std::size_t count) {
  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open kd-tree bundle: " + filepath);
  }
  const std::vector<BundleTocEntry> toc = read_bundle_toc(in, filepath);
  if (toc.empty()) {
    return {};
  }

  const std::size_t start =
      count >= toc.size() ? 0 : toc.size() - count;

  std::vector<KdTreeBundleEntry> result;
  result.reserve(toc.size() - start);
  for (std::size_t i = start; i < toc.size(); ++i) {
    const auto& row = toc[i];
    result.push_back({row.name, load_tree_at_offset(filepath, row.offset, row.size)});
  }
  return result;
}

double KdTree3d::nearest_squared_distance(const Point& query) const {
  double best_dist2 = std::numeric_limits<double>::infinity();

  const auto dist2 = [&](const Point& a) {
    const double dx = a[0] - query[0];
    const double dy = a[1] - query[1];
    const double dz = a[2] - query[2];
    return dx * dx + dy * dy + dz * dz;
  };

  const auto search = [&](const auto& self, const int node_idx) -> void {
    if (node_idx < 0 || static_cast<std::size_t>(node_idx) >= nodes_.size()) {
      return;
    }
    const Node& node = nodes_[static_cast<std::size_t>(node_idx)];
    if (node.axis < 0) {
      best_dist2 = std::min(best_dist2, dist2(points_[node.point_index]));
      return;
    }

    const int axis = node.axis;
    const double q = query[static_cast<std::size_t>(axis)];
    const int first = (q < node.split) ? node.left : node.right;
    const int second = (first == node.left) ? node.right : node.left;
    self(self, first);

    const double diff = q - node.split;
    if (diff * diff < best_dist2) {
      self(self, second);
    }
  };

  search(search, 0);
  return best_dist2;
}

std::size_t KdTree3d::nearest_index(const Point& query) const {
  double best_dist2 = std::numeric_limits<double>::infinity();
  std::size_t best_index = 0;

  const auto dist2 = [&](const Point& a) {
    const double dx = a[0] - query[0];
    const double dy = a[1] - query[1];
    const double dz = a[2] - query[2];
    return dx * dx + dy * dy + dz * dz;
  };

  const auto search = [&](const auto& self, const int node_idx) -> void {
    if (node_idx < 0 || static_cast<std::size_t>(node_idx) >= nodes_.size()) {
      return;
    }
    const Node& node = nodes_[static_cast<std::size_t>(node_idx)];
    if (node.axis < 0) {
      const double d2 = dist2(points_[node.point_index]);
      if (d2 < best_dist2) {
        best_dist2 = d2;
        best_index = node.point_index;
      }
      return;
    }

    const int axis = node.axis;
    const double q = query[static_cast<std::size_t>(axis)];
    const int first = (q < node.split) ? node.left : node.right;
    const int second = (first == node.left) ? node.right : node.left;
    self(self, first);

    const double diff = q - node.split;
    if (diff * diff < best_dist2) {
      self(self, second);
    }
  };

  search(search, 0);
  return best_index;
}

}  // namespace tin_gen
