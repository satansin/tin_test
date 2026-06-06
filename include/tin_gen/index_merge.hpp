#pragma once

#include "tin_gen/kd_tree.hpp"
#include "tin_gen/rs_tree.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tin_gen {

inline constexpr const char* kRsMergeManifestFilename = "rs_merge_manifest.txt";
inline constexpr const char* kKdMergeManifestFilename = "kd_merge_manifest.txt";
inline constexpr const char* kRsMergeBundleExtension = ".tinrs";
inline constexpr const char* kKdMergeBundleExtension = ".tinkd";
inline constexpr std::size_t kDefaultMaxIndexesPerBundle = 5000;

struct IndexMergeEntry {
  std::string bundle_file;
  std::size_t mesh_index = 0;
  std::string mesh_name;
  std::uint64_t offset_bytes = 0;
  std::uint64_t size_bytes = 0;
};

struct IndexMergeManifest {
  std::filesystem::path source_dir;
  std::filesystem::path manifest_path;
  std::size_t max_indexes_per_bundle = kDefaultMaxIndexesPerBundle;
  std::size_t mesh_count = 0;
  std::size_t bundle_count = 0;
  std::vector<IndexMergeEntry> entries;
};

[[nodiscard]] std::optional<std::filesystem::path> find_rs_index_merge_manifest(
    const std::filesystem::path& dir);

[[nodiscard]] std::optional<std::filesystem::path> find_kd_index_merge_manifest(
    const std::filesystem::path& dir);

[[nodiscard]] IndexMergeManifest load_rs_index_merge_manifest(
    const std::filesystem::path& manifest_path);

[[nodiscard]] IndexMergeManifest load_kd_index_merge_manifest(
    const std::filesystem::path& manifest_path);

struct IndexBundleWrittenInfo {
  std::string bundle_file;
  std::size_t index_count = 0;
  std::size_t file_size_bytes = 0;
  std::size_t first_mesh_index = 0;  // 0-based
  std::size_t last_mesh_index = 0;
  double write_wall_seconds = 0.0;
  double write_cpu_seconds = 0.0;
};

using IndexBundleWrittenCallback = std::function<void(const IndexBundleWrittenInfo&)>;

/// Writes up to @p max_indexes_per_bundle trees per `merged_NNN.tinrs` plus a manifest.
class RsIndexMergeWriter {
 public:
  RsIndexMergeWriter(std::filesystem::path output_dir, std::filesystem::path source_dir,
                     std::size_t max_indexes_per_bundle = kDefaultMaxIndexesPerBundle);

  void set_bundle_written_callback(IndexBundleWrittenCallback callback);
  void add(std::size_t mesh_index, std::string mesh_name, RsTree3d tree);
  void finish();

  [[nodiscard]] std::size_t mesh_count() const { return mesh_count_; }
  [[nodiscard]] std::size_t bundle_count() const { return bundle_count_; }

 private:
  void flush_batch();

  std::filesystem::path output_dir_;
  std::filesystem::path source_dir_;
  std::size_t max_indexes_per_bundle_ = kDefaultMaxIndexesPerBundle;
  std::size_t bundle_index_ = 0;
  std::size_t mesh_count_ = 0;
  std::size_t bundle_count_ = 0;
  std::vector<IndexMergeEntry> manifest_entries_;
  std::vector<RsTreeBundleEntry> batch_;
  std::vector<std::size_t> pending_mesh_indices_;
  IndexBundleWrittenCallback bundle_written_callback_;
};

/// Writes up to @p max_indexes_per_bundle trees per `merged_NNN.tinkd` plus a manifest.
class KdIndexMergeWriter {
 public:
  KdIndexMergeWriter(std::filesystem::path output_dir, std::filesystem::path source_dir,
                     std::size_t max_indexes_per_bundle = kDefaultMaxIndexesPerBundle);

  void set_bundle_written_callback(IndexBundleWrittenCallback callback);
  void add(std::size_t mesh_index, std::string mesh_name, KdTree3d tree);
  void finish();

  [[nodiscard]] std::size_t mesh_count() const { return mesh_count_; }
  [[nodiscard]] std::size_t bundle_count() const { return bundle_count_; }

 private:
  void flush_batch();

  std::filesystem::path output_dir_;
  std::filesystem::path source_dir_;
  std::size_t max_indexes_per_bundle_ = kDefaultMaxIndexesPerBundle;
  std::size_t bundle_index_ = 0;
  std::size_t mesh_count_ = 0;
  std::size_t bundle_count_ = 0;
  std::vector<IndexMergeEntry> manifest_entries_;
  std::vector<KdTreeBundleEntry> batch_;
  std::vector<std::size_t> pending_mesh_indices_;
  IndexBundleWrittenCallback bundle_written_callback_;
};

}  // namespace tin_gen
