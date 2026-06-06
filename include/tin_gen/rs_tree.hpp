#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace tin_gen {

/// Bulk-loaded 3D R*-tree (STR packing) over a static point set.
class RsTree3d {
 public:
  using Point = std::array<double, 3>;

  RsTree3d() = default;
  explicit RsTree3d(std::vector<Point> points);

  [[nodiscard]] const std::vector<Point>& points() const { return points_; }

  /// Write binary index (magic TINRSV1) to a .rstree file.
  void save(const std::string& filepath) const;

  /// Load index from disk.
  [[nodiscard]] static RsTree3d load(const std::string& filepath);

  /// Serialize tree payload (TINRSV1, including magic) to a byte buffer.
  [[nodiscard]] std::vector<std::uint8_t> serialize() const;

  /// Parse a buffer written by serialize().
  [[nodiscard]] static RsTree3d deserialize(const std::vector<std::uint8_t>& bytes);

  /// Squared Euclidean distance to the closest point in the tree.
  [[nodiscard]] double nearest_squared_distance(const Point& query) const;

 private:
  struct Mbr {
    double min[3]{};
    double max[3]{};
  };

  struct Entry {
    Mbr mbr{};
    int payload = -1;  // point index (leaf) or child node index (internal)
  };

  struct Node {
    bool leaf = true;
    std::vector<Entry> entries;
  };

  static constexpr int kMaxEntries = 16;
  static constexpr int kMinEntries = 8;

  std::vector<Point> points_;
  std::vector<Node> nodes_;
  int root_index_ = -1;

  [[nodiscard]] static Mbr mbr_from_point(const Point& point);
  [[nodiscard]] static Mbr mbr_union(const Mbr& a, const Mbr& b);
  [[nodiscard]] static double point_to_mbr_squared_distance(const Point& query, const Mbr& box);
  [[nodiscard]] static Mbr mbr_of_entries(const std::vector<Entry>& entries);

  [[nodiscard]] Entry make_node_entry(int node_index) const;
  int build_level(const std::vector<int>& node_indices);

  void write_body(std::ostream& out) const;
  void read_body(std::istream& in);
};

struct RsTreeBundleEntry {
  std::string name;
  RsTree3d tree;
};

void save_rs_tree_bundle(const std::string& filepath, const std::vector<RsTreeBundleEntry>& entries);

[[nodiscard]] std::vector<RsTreeBundleEntry> load_rs_tree_bundle(const std::string& filepath);

enum class RsTreeFolderLoadSource { Bundle, PerFile };

struct LoadRsTreesFromFolderResult {
  RsTreeFolderLoadSource source = RsTreeFolderLoadSource::PerFile;
  std::optional<std::filesystem::path> manifest_path;
  std::unordered_map<std::string, RsTree3d> trees_by_stem;
};

[[nodiscard]] bool is_rs_tree_bundle_file(const std::filesystem::path& path);

void verify_rs_tree_vertex_count(std::size_t expected_vertex_count, const RsTree3d& tree,
                                 const std::string& context);

/// Load R*-trees for @p ply_files from @p rs_dir: split manifest bundles first, else `<stem>.rstree`.
[[nodiscard]] LoadRsTreesFromFolderResult load_rs_trees_from_folder(
    const std::filesystem::path& rs_dir,
    const std::vector<std::filesystem::path>& ply_files,
    const std::vector<std::size_t>& expected_vertex_counts);

}  // namespace tin_gen
