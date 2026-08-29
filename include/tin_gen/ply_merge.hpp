#pragma once

#include "tin_gen/tin_mesh.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tin_gen {

struct DatasetMeshListing;

inline constexpr const char* kPlyMergeManifestFilename = "ply_merge_manifest.txt";
inline constexpr const char* kPlyMergeBundleExtension = ".tinply";
inline constexpr const char* kPackSettingFilename = "pack.setting";
inline constexpr const char* kDatasetsNormDirectoryName = "datasets_norm";
inline constexpr const char* kDatasetsNormPackDirectoryName = "datasets_norm_pack";
inline constexpr std::size_t kDefaultMaxMeshesPerPlyBundle = 5000;

// --- Manifest records ---

struct PlyMergeEntry {
  std::string bundle_file;
  std::size_t mesh_index = 0;
  std::string original_filename;
  std::uint64_t offset_bytes = 0;
  std::uint64_t size_bytes = 0;
};

struct PlyMergeManifest {
  std::filesystem::path source_dir;
  std::filesystem::path manifest_path;
  std::size_t max_meshes_per_bundle = kDefaultMaxMeshesPerPlyBundle;
  std::size_t mesh_count = 0;
  std::size_t bundle_count = 0;
  std::vector<PlyMergeEntry> entries;
};

struct PlyMergeOptions {
  std::size_t max_meshes_per_bundle = kDefaultMaxMeshesPerPlyBundle;
  std::size_t max_objects = 0;  // 0 = all
};

struct PlyMergeResult {
  std::size_t mesh_count = 0;
  std::size_t bundle_count = 0;
  std::filesystem::path manifest_path;
};

/// Incrementally write generated PLY meshes directly into bundle files.
class PlyMergeWriter {
 public:
  PlyMergeWriter(std::filesystem::path output_dir, std::filesystem::path source_dir,
                 std::size_t max_meshes_per_bundle = kDefaultMaxMeshesPerPlyBundle);
  ~PlyMergeWriter();

  PlyMergeWriter(const PlyMergeWriter&) = delete;
  PlyMergeWriter& operator=(const PlyMergeWriter&) = delete;

  /// Add one mesh using @p original_filename as its manifest name.
  void add(std::string original_filename, const TinMesh& mesh);

  /// Close the current bundle and write the manifest.
  [[nodiscard]] PlyMergeResult finish();

 private:
  void open_bundle();
  void close_bundle();

  std::filesystem::path output_dir_;
  std::filesystem::path source_dir_;
  std::size_t max_meshes_per_bundle_;
  std::size_t bundle_index_ = 0;
  std::size_t bundle_count_ = 0;
  std::size_t meshes_in_bundle_ = 0;
  std::uint64_t bundle_offset_ = 0;
  std::ofstream bundle_out_;
  std::vector<PlyMergeEntry> entries_;
  bool finished_ = false;
};

// --- Merge / read API ---

/// Parse a manifest written by write_ply_merge().
[[nodiscard]] PlyMergeManifest load_ply_merge_manifest(const std::filesystem::path& manifest_path);

/// Merge `.ply` files from @p source_dir into bundle files under @p output_dir and write
/// @p kPlyMergeManifestFilename.
[[nodiscard]] PlyMergeResult write_ply_merge(const std::filesystem::path& source_dir,
                                             const std::filesystem::path& output_dir,
                                             PlyMergeOptions opts = {});

/// Read one mesh from a merged dataset using @p manifest_path (or its parent directory).
[[nodiscard]] TinMesh read_ply_from_merge(const std::filesystem::path& manifest_path,
                                          std::string_view original_filename);

/// Read one mesh by 0-based index from the manifest order.
[[nodiscard]] TinMesh read_ply_from_merge_by_index(const std::filesystem::path& manifest_path,
                                                   std::size_t mesh_index);

struct PackBundleLoadedInfo {
  std::string bundle_file;
  std::size_t size_bytes = 0;
  double read_wall_seconds = 0.0;
  double read_cpu_seconds = 0.0;
  std::size_t mesh_index = 0;  // 0-based mesh that triggered the load
};

using PackBundleLoadedCallback = std::function<void(const PackBundleLoadedInfo&)>;

/// Efficient pack reader: manifest and each bundle file are read from storage once.
class PlyMergeDatasetReader {
 public:
  explicit PlyMergeDatasetReader(const std::filesystem::path& manifest_path,
                                 std::size_t max_objects = 0);

  [[nodiscard]] std::size_t mesh_count() const { return entries_.size(); }
  [[nodiscard]] const PlyMergeManifest& manifest() const { return manifest_; }
  [[nodiscard]] const std::vector<PlyMergeEntry>& entries() const { return entries_; }

  /// Build a dataset listing (paths / bundle names) without re-reading the manifest.
  [[nodiscard]] DatasetMeshListing make_listing(const std::filesystem::path& input_dir) const;

  /// Called after each bundle file is read from disk into memory.
  void set_bundle_loaded_callback(PackBundleLoadedCallback callback);

  /// Read mesh @p index using the in-memory bundle buffer for its bundle file.
  [[nodiscard]] TinMesh read_mesh(std::size_t index, PlyReadContent content = PlyReadContent::Full);

  /// Drop the in-memory bundle buffer after all meshes in the bundle are processed.
  void release_loaded_bundle();

 private:
  void ensure_bundle_loaded(const std::string& bundle_file, std::size_t mesh_index);
  [[nodiscard]] TinMesh read_mesh_from_entry(const PlyMergeEntry& entry, std::size_t mesh_index,
                                             PlyReadContent content = PlyReadContent::Full);
  void report_bundle_loaded(const PackBundleLoadedInfo& info);

  PlyMergeManifest manifest_;
  std::vector<PlyMergeEntry> entries_;
  std::string loaded_bundle_file_;
  std::vector<char> loaded_bundle_bytes_;
  PackBundleLoadedCallback bundle_loaded_callback_;
};

// --- Pack discovery ---

/// If @p input_dir has pack metadata, returns the manifest path; otherwise nullopt.
/// Checks, in order: `pack.setting`, manifest in @p input_dir, parallel `datasets_norm_pack/<name>/`.
[[nodiscard]] std::optional<std::filesystem::path> find_pack_manifest_for_dataset(
    const std::filesystem::path& input_dir);

}  // namespace tin_gen
