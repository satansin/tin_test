#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tin_gen {

// --- Mesh format (generate / write) ---

enum class MeshFormat { Ply, Obj };

[[nodiscard]] MeshFormat parse_mesh_format(std::string_view value);
[[nodiscard]] std::string_view mesh_format_extension(MeshFormat format);
[[nodiscard]] std::string_view mesh_format_name(MeshFormat format);

// --- Mesh folder listing and ordering (normalize / kdvertices / pairwise_distance) ---

/// If @p filename matches `prefix_NUMBER.ext` (number is the stem suffix after the last `_`),
/// returns NUMBER; otherwise nullopt.
[[nodiscard]] std::optional<std::uint64_t> mesh_filename_trailing_index(std::string_view filename);

/// Sort key for mesh paths: numeric trailing index when both names match the pattern, else
/// lexicographic by filename.
[[nodiscard]] bool compare_mesh_paths_by_filename(const std::filesystem::path& a,
                                                  const std::filesystem::path& b);

void sort_mesh_paths_by_filename(std::vector<std::filesystem::path>& paths);

struct ListPlyFilesOptions {
  std::size_t max_objects = 0;
  /// When true, use @p metadata_path if set, else `<input-dir>/metadata.txt` when present.
  bool use_metadata = true;
  std::optional<std::filesystem::path> metadata_path;
};

/// Resolve metadata CSV path from options (explicit path or default under input dir).
[[nodiscard]] std::optional<std::filesystem::path> resolve_metadata_path(
    const std::filesystem::path& input_dir, const ListPlyFilesOptions& opts);

/// Read mesh filenames in metadata file order (CSV first column: mesh_name).
[[nodiscard]] std::vector<std::string> read_metadata_mesh_list(const std::filesystem::path& path);

/// List `.ply` files in @p input_dir. With metadata: metadata row order; otherwise numeric
/// `name_NUMBER.ply` order (then lexicographic). Applies @p opts.max_objects after ordering.
[[nodiscard]] std::vector<std::filesystem::path> list_ply_files_in_directory(
    const std::filesystem::path& input_dir, ListPlyFilesOptions opts = {});

// --- Folder mesh load progress (normalize / kdvertices / pairwise_distance) ---

inline constexpr std::size_t kFolderMeshLoadProgressInterval = 2000;

/// Current process resident memory (best effort; 0 if unavailable).
[[nodiscard]] std::size_t current_process_resident_bytes();

/// Reports @p loaded / @p total, elapsed seconds, and resident memory to stdout.
void report_folder_mesh_load_progress(std::size_t loaded, std::size_t total,
                                      double elapsed_seconds, std::string_view label);

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
