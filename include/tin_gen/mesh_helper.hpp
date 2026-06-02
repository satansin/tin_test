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
