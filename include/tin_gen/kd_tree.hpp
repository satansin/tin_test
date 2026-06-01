#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace tin_gen {

/// 3D KD-tree over a point set (built for nearest-neighbor queries).
class KdTree3d {
 public:
  using Point = std::array<double, 3>;

  KdTree3d() = default;
  explicit KdTree3d(std::vector<Point> points);

  [[nodiscard]] const std::vector<Point>& points() const { return points_; }

  /// Write binary index (magic TINKDV1) to a .kdtree file.
  void save(const std::string& filepath) const;

  /// Load index from disk.
  [[nodiscard]] static KdTree3d load(const std::string& filepath);

  /// Serialize tree payload (TINKDV1, including magic) to a byte buffer.
  [[nodiscard]] std::vector<std::uint8_t> serialize() const;

  /// Parse a buffer written by serialize().
  [[nodiscard]] static KdTree3d deserialize(const std::vector<std::uint8_t>& bytes);

  /// Nearest neighbor index for query point (for validation / future use).
  [[nodiscard]] std::size_t nearest_index(const Point& query) const;

  /// Squared Euclidean distance to the closest point in the tree.
  [[nodiscard]] double nearest_squared_distance(const Point& query) const;

 private:
  struct Node {
    int axis = -1;  // 0=x,1=y,2=z; leaf if axis < 0
    std::size_t point_index = 0;
    float split = 0.0f;
    int left = -1;
    int right = -1;
  };

  std::vector<Point> points_;
  std::vector<Node> nodes_;

  int build_recursive(std::vector<std::size_t>& indices, int depth);

  void write_body(std::ostream& out) const;
  void read_body(std::istream& in);
};

/// One named KD-tree inside a combined bundle file.
struct KdTreeBundleEntry {
  std::string name;
  KdTree3d tree;
};

/// Write all trees to one binary file (magic TINKDB1) with a table of contents.
void save_kd_tree_bundle(const std::string& filepath,
                         const std::vector<KdTreeBundleEntry>& entries);

/// Load every tree from a combined bundle file.
[[nodiscard]] std::vector<KdTreeBundleEntry> load_kd_tree_bundle(const std::string& filepath);

/// Load only the last @p count trees (efficient when @p count is small).
[[nodiscard]] std::vector<KdTreeBundleEntry> load_kd_tree_bundle_last(const std::string& filepath,
                                                                     std::size_t count);

/// Default merged bundle filename written by `kdvertices --combined`.
inline constexpr const char* kDefaultKdTreeBundleFilename = "combined.kdtree";

enum class KdTreeFolderLoadSource { Bundle, PerFile };

struct LoadKdTreesFromFolderOptions {
  /// Merged bundle filename to try first under the kd-tree directory.
  std::string bundle_filename = kDefaultKdTreeBundleFilename;
};

struct LoadKdTreesFromFolderResult {
  KdTreeFolderLoadSource source = KdTreeFolderLoadSource::PerFile;
  std::optional<std::filesystem::path> bundle_path;
  /// Mesh stem (e.g. `object_1`) -> tree.
  std::unordered_map<std::string, KdTree3d> trees_by_stem;
};

[[nodiscard]] bool is_kdtree_bundle_file(const std::filesystem::path& path);

/// Returns the merged bundle path when present (`TINKDB1`), else nullopt.
[[nodiscard]] std::optional<std::filesystem::path> find_kdtree_bundle_in_directory(
    const std::filesystem::path& dir,
    std::string_view bundle_filename = kDefaultKdTreeBundleFilename);

/// Throws if @p tree point count differs from @p expected_vertex_count.
void verify_kdtree_vertex_count(std::size_t expected_vertex_count, const KdTree3d& tree,
                                const std::string& context);

/// Load KD-trees for @p ply_files from @p kdtree_dir: merged bundle first, else `<stem>.kdtree`.
/// @p expected_vertex_counts must match @p ply_files size; each loaded tree is checked against it.
[[nodiscard]] LoadKdTreesFromFolderResult load_kdtrees_from_folder(
    const std::filesystem::path& kdtree_dir,
    const std::vector<std::filesystem::path>& ply_files,
    const std::vector<std::size_t>& expected_vertex_counts,
    LoadKdTreesFromFolderOptions opts = {});

}  // namespace tin_gen
