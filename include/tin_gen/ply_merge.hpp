#pragma once

#include "tin_gen/tin_mesh.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tin_gen {

inline constexpr const char* kPlyMergeManifestFilename = "ply_merge_manifest.txt";
inline constexpr const char* kPlyMergeBundleExtension = ".tinply";
inline constexpr const char* kPackSettingFilename = "pack.setting";
inline constexpr const char* kDatasetsNormDirectoryName = "datasets_norm";
inline constexpr const char* kDatasetsNormPackDirectoryName = "datasets_norm_pack";
inline constexpr std::size_t kDefaultMaxMeshesPerPlyBundle = 5000;

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

/// If @p input_dir has pack metadata, returns the manifest path; otherwise nullopt.
/// Checks, in order: `pack.setting`, manifest in @p input_dir, parallel `datasets_norm_pack/<name>/`.
[[nodiscard]] std::optional<std::filesystem::path> find_pack_manifest_for_dataset(
    const std::filesystem::path& input_dir);

}  // namespace tin_gen
