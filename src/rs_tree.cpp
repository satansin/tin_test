#include "tin_gen/rs_tree.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

constexpr int kDims = 3;
constexpr char kMagic[8] = {'T', 'I', 'N', 'R', 'S', 'V', '1', '\0'};
constexpr char kBundleMagic[8] = {'T', 'I', 'N', 'R', 'S', 'B', '1', '\0'};

void write_u64(std::ostream& out, const std::uint64_t v) {
  out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

void write_i32(std::ostream& out, const std::int32_t v) {
  out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

void write_u8(std::ostream& out, const std::uint8_t v) {
  out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

void write_f32(std::ostream& out, const float v) {
  out.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

std::uint64_t read_u64(std::istream& in) {
  std::uint64_t v = 0;
  in.read(reinterpret_cast<char*>(&v), sizeof(v));
  if (!in) {
    throw std::runtime_error("Unexpected EOF reading R*-tree file");
  }
  return v;
}

std::int32_t read_i32(std::istream& in) {
  std::int32_t v = 0;
  in.read(reinterpret_cast<char*>(&v), sizeof(v));
  if (!in) {
    throw std::runtime_error("Unexpected EOF reading R*-tree file");
  }
  return v;
}

std::uint8_t read_u8(std::istream& in) {
  std::uint8_t v = 0;
  in.read(reinterpret_cast<char*>(&v), sizeof(v));
  if (!in) {
    throw std::runtime_error("Unexpected EOF reading R*-tree file");
  }
  return v;
}

float read_f32(std::istream& in) {
  float v = 0.0f;
  in.read(reinterpret_cast<char*>(&v), sizeof(v));
  if (!in) {
    throw std::runtime_error("Unexpected EOF reading R*-tree file");
  }
  return v;
}

struct BundleTocEntry {
  std::string name;
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
};

void read_bundle_magic(std::istream& in, const std::string& filepath) {
  char magic[8]{};
  in.read(magic, 8);
  if (std::string(magic, 8) != std::string(kBundleMagic, 8)) {
    throw std::runtime_error("Not a TINRSB1 R*-tree bundle: " + filepath);
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

RsTree3d load_tree_at_offset(const std::string& filepath, const std::uint64_t offset,
                                const std::uint64_t size) {
  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open R*-tree bundle: " + filepath);
  }
  in.seekg(static_cast<std::streamoff>(offset));
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
  if (!in) {
    throw std::runtime_error("Unexpected EOF reading R*-tree bundle blob: " + filepath);
  }
  return RsTree3d::deserialize(bytes);
}

}  // namespace

RsTree3d::Mbr RsTree3d::mbr_from_point(const Point& point) {
  Mbr box{};
  for (int axis = 0; axis < kDims; ++axis) {
    box.min[axis] = point[static_cast<std::size_t>(axis)];
    box.max[axis] = point[static_cast<std::size_t>(axis)];
  }
  return box;
}

RsTree3d::Mbr RsTree3d::mbr_union(const Mbr& a, const Mbr& b) {
  Mbr out = a;
  for (int axis = 0; axis < kDims; ++axis) {
    out.min[axis] = std::min(out.min[axis], b.min[axis]);
    out.max[axis] = std::max(out.max[axis], b.max[axis]);
  }
  return out;
}

RsTree3d::Mbr RsTree3d::mbr_of_entries(const std::vector<Entry>& entries) {
  Mbr box = entries.front().mbr;
  for (std::size_t i = 1; i < entries.size(); ++i) {
    box = mbr_union(box, entries[i].mbr);
  }
  return box;
}

double RsTree3d::point_to_mbr_squared_distance(const Point& query, const Mbr& box) {
  double dist2 = 0.0;
  for (int axis = 0; axis < kDims; ++axis) {
    const double coord = query[static_cast<std::size_t>(axis)];
    if (coord < box.min[axis]) {
      const double d = box.min[axis] - coord;
      dist2 += d * d;
    } else if (coord > box.max[axis]) {
      const double d = coord - box.max[axis];
      dist2 += d * d;
    }
  }
  return dist2;
}

RsTree3d::Entry RsTree3d::make_node_entry(const int node_index) const {
  return Entry{mbr_of_entries(nodes_[static_cast<std::size_t>(node_index)].entries), node_index};
}

int RsTree3d::build_level(const std::vector<int>& node_indices) {
  if (node_indices.empty()) {
    throw std::runtime_error("R*-tree build_level called with no nodes");
  }
  if (static_cast<int>(node_indices.size()) <= kMaxEntries) {
    Node parent;
    parent.leaf = false;
    parent.entries.reserve(node_indices.size());
    for (const int child_index : node_indices) {
      parent.entries.push_back(make_node_entry(child_index));
    }
    nodes_.push_back(std::move(parent));
    return static_cast<int>(nodes_.size()) - 1;
  }

  std::vector<int> sorted = node_indices;
  std::sort(sorted.begin(), sorted.end(), [&](const int a, const int b) {
    const Mbr& mbr_a = make_node_entry(a).mbr;
    const Mbr& mbr_b = make_node_entry(b).mbr;
    if (mbr_a.min[0] != mbr_b.min[0]) {
      return mbr_a.min[0] < mbr_b.min[0];
    }
    if (mbr_a.min[1] != mbr_b.min[1]) {
      return mbr_a.min[1] < mbr_b.min[1];
    }
    return mbr_a.min[2] < mbr_b.min[2];
  });

  const int group_count = static_cast<int>(
      std::ceil(static_cast<double>(sorted.size()) / static_cast<double>(kMaxEntries)));
  const int base_size = static_cast<int>(sorted.size()) / group_count;
  const int remainder = static_cast<int>(sorted.size()) % group_count;

  std::vector<int> parents;
  parents.reserve(static_cast<std::size_t>(group_count));

  std::size_t offset = 0;
  for (int group = 0; group < group_count; ++group) {
    const std::size_t group_size =
        static_cast<std::size_t>(base_size + (group < remainder ? 1 : 0));
    std::vector<int> children(sorted.begin() + static_cast<std::ptrdiff_t>(offset),
                              sorted.begin() + static_cast<std::ptrdiff_t>(offset + group_size));
    offset += group_size;
    parents.push_back(build_level(children));
  }

  return build_level(parents);
}

RsTree3d::RsTree3d(std::vector<Point> points) : points_(std::move(points)) {
  if (points_.empty()) {
    throw std::runtime_error("Cannot build R*-tree from 0 points");
  }

  std::vector<int> point_indices(points_.size());
  for (std::size_t i = 0; i < point_indices.size(); ++i) {
    point_indices[static_cast<std::size_t>(i)] = static_cast<int>(i);
  }

  std::sort(point_indices.begin(), point_indices.end(), [&](const int a, const int b) {
    const Point& pa = points_[static_cast<std::size_t>(a)];
    const Point& pb = points_[static_cast<std::size_t>(b)];
    if (pa[0] != pb[0]) {
      return pa[0] < pb[0];
    }
    if (pa[1] != pb[1]) {
      return pa[1] < pb[1];
    }
    return pa[2] < pb[2];
  });

  const int leaf_count = static_cast<int>(
      std::ceil(static_cast<double>(point_indices.size()) / static_cast<double>(kMaxEntries)));
  const int base_leaf_size = static_cast<int>(point_indices.size()) / leaf_count;
  const int leaf_remainder = static_cast<int>(point_indices.size()) % leaf_count;

  std::vector<int> leaf_nodes;
  leaf_nodes.reserve(static_cast<std::size_t>(leaf_count));

  std::size_t offset = 0;
  for (int leaf = 0; leaf < leaf_count; ++leaf) {
    const std::size_t leaf_size =
        static_cast<std::size_t>(base_leaf_size + (leaf < leaf_remainder ? 1 : 0));

    Node leaf_node;
    leaf_node.leaf = true;
    leaf_node.entries.reserve(leaf_size);
    for (std::size_t i = 0; i < leaf_size; ++i) {
      const int point_index =
          point_indices[static_cast<std::size_t>(static_cast<std::ptrdiff_t>(offset + i))];
      leaf_node.entries.push_back(
          Entry{mbr_from_point(points_[static_cast<std::size_t>(point_index)]), point_index});
    }
    offset += leaf_size;

    nodes_.push_back(std::move(leaf_node));
    leaf_nodes.push_back(static_cast<int>(nodes_.size()) - 1);
  }

  root_index_ = build_level(leaf_nodes);
}

double RsTree3d::nearest_squared_distance(const Point& query) const {
  if (root_index_ < 0) {
    throw std::runtime_error("R*-tree is empty");
  }

  double best_dist2 = std::numeric_limits<double>::infinity();

  struct QueueItem {
    double dist2;
    int node_index;
    int entry_index;
  };
  struct QueueCompare {
    bool operator()(const QueueItem& a, const QueueItem& b) const { return a.dist2 > b.dist2; }
  };

  std::priority_queue<QueueItem, std::vector<QueueItem>, QueueCompare> queue;

  const auto enqueue_node = [&](const int node_index) {
    const Node& node = nodes_[static_cast<std::size_t>(node_index)];
    for (int i = 0; i < static_cast<int>(node.entries.size()); ++i) {
      queue.push({point_to_mbr_squared_distance(query, node.entries[static_cast<std::size_t>(i)].mbr),
                  node_index, i});
    }
  };

  enqueue_node(root_index_);

  while (!queue.empty() && queue.top().dist2 < best_dist2) {
    const QueueItem item = queue.top();
    queue.pop();

    const Node& node = nodes_[static_cast<std::size_t>(item.node_index)];
    const Entry& entry = node.entries[static_cast<std::size_t>(item.entry_index)];

    if (node.leaf) {
      const Point& candidate = points_[static_cast<std::size_t>(entry.payload)];
      double dist2 = 0.0;
      for (int axis = 0; axis < kDims; ++axis) {
        const double d =
            candidate[static_cast<std::size_t>(axis)] - query[static_cast<std::size_t>(axis)];
        dist2 += d * d;
      }
      best_dist2 = std::min(best_dist2, dist2);
      continue;
    }

    const int child_index = entry.payload;
    const Node& child = nodes_[static_cast<std::size_t>(child_index)];
    for (int i = 0; i < static_cast<int>(child.entries.size()); ++i) {
      const Entry& child_entry = child.entries[static_cast<std::size_t>(i)];
      const double dist2 = point_to_mbr_squared_distance(query, child_entry.mbr);
      if (dist2 >= best_dist2) {
        continue;
      }
      if (child.leaf) {
        const Point& candidate = points_[static_cast<std::size_t>(child_entry.payload)];
        double point_dist2 = 0.0;
        for (int axis = 0; axis < kDims; ++axis) {
          const double d =
              candidate[static_cast<std::size_t>(axis)] - query[static_cast<std::size_t>(axis)];
          point_dist2 += d * d;
        }
        best_dist2 = std::min(best_dist2, point_dist2);
      } else {
        queue.push({dist2, child_index, i});
      }
    }
  }

  return best_dist2;
}

void RsTree3d::write_body(std::ostream& out) const {
  write_u64(out, static_cast<std::uint64_t>(points_.size()));
  for (const auto& p : points_) {
    const float xyz[3] = {static_cast<float>(p[0]), static_cast<float>(p[1]),
                          static_cast<float>(p[2])};
    out.write(reinterpret_cast<const char*>(xyz), sizeof(xyz));
  }

  write_i32(out, root_index_);
  write_u64(out, static_cast<std::uint64_t>(nodes_.size()));
  for (const auto& node : nodes_) {
    write_u8(out, node.leaf ? 1 : 0);
    write_u64(out, static_cast<std::uint64_t>(node.entries.size()));
    for (const auto& entry : node.entries) {
      for (int axis = 0; axis < kDims; ++axis) {
        write_f32(out, static_cast<float>(entry.mbr.min[axis]));
        write_f32(out, static_cast<float>(entry.mbr.max[axis]));
      }
      write_i32(out, entry.payload);
    }
  }
}

void RsTree3d::read_body(std::istream& in) {
  points_.resize(static_cast<std::size_t>(read_u64(in)));
  for (auto& p : points_) {
    float xyz[3];
    in.read(reinterpret_cast<char*>(xyz), sizeof(xyz));
    if (!in) {
      throw std::runtime_error("Unexpected EOF reading R*-tree vertices");
    }
    p = {xyz[0], xyz[1], xyz[2]};
  }

  root_index_ = read_i32(in);
  nodes_.resize(static_cast<std::size_t>(read_u64(in)));
  for (auto& node : nodes_) {
    node.leaf = read_u8(in) != 0;
    node.entries.resize(static_cast<std::size_t>(read_u64(in)));
    for (auto& entry : node.entries) {
      for (int axis = 0; axis < kDims; ++axis) {
        entry.mbr.min[axis] = read_f32(in);
        entry.mbr.max[axis] = read_f32(in);
      }
      entry.payload = read_i32(in);
    }
  }

  if (points_.empty() || nodes_.empty() || root_index_ < 0) {
    throw std::runtime_error("Invalid empty R*-tree data");
  }
}

void RsTree3d::save(const std::string& filepath) const {
  std::ofstream out(filepath, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Failed to open R*-tree file for writing: " + filepath);
  }
  out.write(kMagic, 8);
  write_body(out);
}

RsTree3d RsTree3d::load(const std::string& filepath) {
  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open R*-tree file for reading: " + filepath);
  }
  char magic[8]{};
  in.read(magic, 8);
  if (std::string(magic, 8) != std::string(kMagic, 8)) {
    throw std::runtime_error("Not a TINRSV1 R*-tree file: " + filepath);
  }
  RsTree3d tree;
  tree.read_body(in);
  return tree;
}

std::vector<std::uint8_t> RsTree3d::serialize() const {
  std::ostringstream out(std::ios::binary);
  out.write(kMagic, 8);
  write_body(out);
  const std::string data = out.str();
  return std::vector<std::uint8_t>(data.begin(), data.end());
}

RsTree3d RsTree3d::deserialize(const std::vector<std::uint8_t>& bytes) {
  if (bytes.size() < 8) {
    throw std::runtime_error("R*-tree buffer too small");
  }
  if (std::memcmp(bytes.data(), kMagic, 8) != 0) {
    throw std::runtime_error("Not a TINRSV1 R*-tree buffer");
  }
  std::istringstream in(std::string(bytes.begin() + 8, bytes.end()), std::ios::binary);
  RsTree3d tree;
  tree.read_body(in);
  return tree;
}

void save_rs_tree_bundle(const std::string& filepath,
                         const std::vector<RsTreeBundleEntry>& entries) {
  if (entries.empty()) {
    throw std::runtime_error("Cannot write empty R*-tree bundle");
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
    throw std::runtime_error("Failed to open R*-tree bundle for writing: " + filepath);
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

std::vector<RsTreeBundleEntry> load_rs_tree_bundle(const std::string& filepath) {
  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open R*-tree bundle: " + filepath);
  }
  const std::vector<BundleTocEntry> toc = read_bundle_toc(in, filepath);

  std::vector<RsTreeBundleEntry> result;
  result.reserve(toc.size());
  for (const auto& row : toc) {
    result.push_back({row.name, load_tree_at_offset(filepath, row.offset, row.size)});
  }
  return result;
}

bool is_rs_tree_bundle_file(const fs::path& path) {
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

std::optional<fs::path> find_rs_tree_bundle_in_directory(const fs::path& dir,
                                                        const std::string_view bundle_filename) {
  if (!fs::exists(dir) || !fs::is_directory(dir)) {
    return std::nullopt;
  }
  const fs::path preferred = dir / std::string(bundle_filename);
  if (is_rs_tree_bundle_file(preferred)) {
    return preferred;
  }
  return std::nullopt;
}

void verify_rs_tree_vertex_count(const std::size_t expected_vertex_count, const RsTree3d& tree,
                                const std::string& context) {
  if (tree.points().size() != expected_vertex_count) {
    throw std::runtime_error("rs vertex count mismatch for " + context + " (expected=" +
                             std::to_string(expected_vertex_count) + ", rs=" +
                             std::to_string(tree.points().size()) + ")");
  }
}

LoadRsTreesFromFolderResult load_rs_trees_from_folder(
    const fs::path& rs_dir, const std::vector<fs::path>& ply_files,
    const std::vector<std::size_t>& expected_vertex_counts, LoadRsTreesFromFolderOptions opts) {
  if (expected_vertex_counts.size() != ply_files.size()) {
    throw std::invalid_argument("load_rs_trees_from_folder: expected_vertex_counts size must match "
                                "ply_files");
  }

  LoadRsTreesFromFolderResult result;

  if (const std::optional<fs::path> bundle_path =
          find_rs_tree_bundle_in_directory(rs_dir, opts.bundle_filename)) {
    result.source = RsTreeFolderLoadSource::Bundle;
    result.bundle_path = bundle_path;
    const std::vector<RsTreeBundleEntry> entries = load_rs_tree_bundle(bundle_path->string());
    for (auto& entry : entries) {
      result.trees_by_stem.emplace(std::move(entry.name), std::move(entry.tree));
    }
  } else {
    result.source = RsTreeFolderLoadSource::PerFile;
    for (const auto& ply_path : ply_files) {
      const std::string stem = ply_path.stem().string();
      const fs::path rs_path = rs_dir / (stem + ".rstree");
      if (!fs::exists(rs_path)) {
        throw std::runtime_error("rs file not found: " + rs_path.string());
      }
      if (is_rs_tree_bundle_file(rs_path)) {
        throw std::runtime_error(
            "expected per-mesh rs file but found bundle (use merged bundle at " +
            std::string(opts.bundle_filename) + "): " + rs_path.string());
      }
      result.trees_by_stem.emplace(stem, RsTree3d::load(rs_path.string()));
    }
  }

  for (std::size_t i = 0; i < ply_files.size(); ++i) {
    const std::string stem = ply_files[i].stem().string();
    const auto tree_it = result.trees_by_stem.find(stem);
    if (tree_it == result.trees_by_stem.end()) {
      throw std::runtime_error("rs missing entry for mesh stem: " + stem);
    }
    const std::string context =
        result.source == RsTreeFolderLoadSource::Bundle
            ? result.bundle_path->string() + " [" + stem + "]"
            : (rs_dir / (stem + ".rstree")).string();
    verify_rs_tree_vertex_count(expected_vertex_counts[i], tree_it->second, context);
  }

  return result;
}

}  // namespace tin_gen
