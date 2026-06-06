#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tin_gen {

struct TinMesh;

enum class PlyReadContent { Full, VerticesOnly };

// --- Mesh format (generate / write) ---

enum class MeshFormat { Ply, Obj };

[[nodiscard]] MeshFormat parse_mesh_format(std::string_view value);
[[nodiscard]] std::string_view mesh_format_extension(MeshFormat format);
[[nodiscard]] std::string_view mesh_format_name(MeshFormat format);

// --- Mesh folder listing and ordering (normalize / kd / rs / pairwise_distance) ---

/// If @p filename matches `prefix_NUMBER.ext` (number is the stem suffix after the last `_`),
/// returns NUMBER; otherwise nullopt.
[[nodiscard]] std::optional<std::uint64_t> mesh_filename_trailing_index(std::string_view filename);

/// Sort key for mesh paths: numeric trailing index when both names match the pattern, else
/// lexicographic by filename.
[[nodiscard]] bool compare_mesh_paths_by_filename(const std::filesystem::path& a,
                                                  const std::filesystem::path& b);

void sort_mesh_paths_by_filename(std::vector<std::filesystem::path>& paths);

/// Normalizes @p ext to a lowercase extension including the leading dot (e.g. `ply` -> `.ply`).
[[nodiscard]] std::string normalize_mesh_extension(std::string_view ext);

struct ListMeshFilesOptions {
  std::size_t max_objects = 0;
  /// File extension filter (default `.ply`). Accepts `ply` or `.ply`.
  std::string extension = ".ply";
};

/// List regular files in @p input_dir with @p opts.extension. Sorted by numeric
/// `name_NUMBER.ext` when applicable, otherwise lexicographic. Applies @p opts.max_objects after
/// ordering.
[[nodiscard]] std::vector<std::filesystem::path> list_mesh_files_in_directory(
    const std::filesystem::path& input_dir, ListMeshFilesOptions opts = {});

// --- Dataset mesh listing (per-file .ply or packed bundles) ---

enum class DatasetMeshSource { PerFile, Pack };

struct DatasetMeshListing {
  DatasetMeshSource source = DatasetMeshSource::PerFile;
  std::filesystem::path input_dir;
  std::filesystem::path pack_manifest;
  std::vector<std::filesystem::path> paths;
  /// When @p source is Pack, bundle file for each mesh in @p paths (same length).
  std::vector<std::string> pack_bundles;
  /// When @p source is Pack, distinct bundle files in mesh order (e.g. merged_000.tinply, …).
  std::vector<std::string> pack_bundle_names;
};

/// List meshes in @p input_dir. Uses pack bundles when `pack.setting` or a parallel
/// `datasets_norm_pack/<name>/` manifest exists; otherwise lists `.ply` files from disk.
[[nodiscard]] DatasetMeshListing list_dataset_meshes(const std::filesystem::path& input_dir,
                                                     ListMeshFilesOptions opts = {});

/// Read mesh @p index from a listing returned by list_dataset_meshes().
[[nodiscard]] TinMesh read_dataset_mesh(const DatasetMeshListing& listing, std::size_t index);

// --- Dataset mesh loading helpers (kd / rs / pairwise_distance) ---

using MeshVertex = std::array<double, 3>;

[[nodiscard]] std::vector<MeshVertex> tin_mesh_vertices(const TinMesh& mesh);

[[nodiscard]] ListMeshFilesOptions ply_list_options(std::size_t max_objects = 0);

/// Like list_dataset_meshes(), but rethrows with a @p command prefix.
[[nodiscard]] DatasetMeshListing list_dataset_meshes_for_command(
    const std::filesystem::path& input_dir, ListMeshFilesOptions opts,
    std::string_view command);

void require_non_empty_mesh(const TinMesh& mesh, std::string_view command,
                            const std::filesystem::path& mesh_path);

void print_dataset_mesh_source(std::ostream& out, const DatasetMeshListing& listing);

struct LoadedDatasetMeshes {
  DatasetMeshListing listing;
  std::vector<TinMesh> meshes;
};

/// List and read every mesh in @p input_dir (pack or per-file) with progress reporting.
[[nodiscard]] LoadedDatasetMeshes load_all_dataset_meshes(
    const std::filesystem::path& input_dir, ListMeshFilesOptions opts,
    std::string_view command, std::string_view progress_label,
    PlyReadContent content = PlyReadContent::Full);

// --- Folder mesh load progress (normalize / kd / rs / pairwise_distance) ---

inline constexpr std::size_t kFolderMeshLoadProgressInterval = 2000;

/// Current process resident memory (best effort; 0 if unavailable).
[[nodiscard]] std::size_t current_process_resident_bytes();

/// Reports @p loaded / @p total, elapsed seconds, and resident memory to stdout.
void report_folder_mesh_load_progress(std::size_t loaded, std::size_t total,
                                      double elapsed_seconds, std::string_view label,
                                      std::string_view bundle = {});

/// Progress reporter for load_all_dataset_meshes(); prints start/bundle/progress lines.
class DatasetMeshLoadProgress {
 public:
  DatasetMeshLoadProgress(const DatasetMeshListing& listing, std::string_view label);

  /// Called after a bundle file has been read from disk; prints load timing and extraction start.
  void on_bundle_loaded(std::string_view bundle_file, std::size_t size_bytes,
                        double read_wall_seconds, double read_cpu_seconds, std::size_t mesh_index);

  /// @p loaded is the number of meshes read so far (1-based, up to total).
  void mark_loaded(std::size_t loaded);

 private:
  const DatasetMeshListing& listing_;
  std::string label_;
  std::size_t total_ = 0;
  std::string active_bundle_;
  std::chrono::steady_clock::time_point start_;
};

/// Call mark_loaded(1..total) after each mesh file is read from disk.
class FolderMeshLoadProgress {
 public:
  FolderMeshLoadProgress(std::size_t total, std::string_view label = "mesh files");

  /// @p loaded is the number of files read so far (1-based, up to total).
  void mark_loaded(std::size_t loaded);

 private:
  std::size_t total_ = 0;
  std::string label_;
  std::chrono::steady_clock::time_point start_;
};

}  // namespace tin_gen
