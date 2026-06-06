#include "tin_gen/mesh_helper.hpp"

#include "tin_gen/ply_merge.hpp"
#include "tin_gen/tin_mesh.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(__APPLE__)
#include <mach/mach.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

std::string to_lower(std::string_view value) {
  std::string out(value);
  std::transform(out.begin(), out.end(), out.begin(),
               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

std::string trim(std::string s) {
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
  while (!s.empty() && is_space(static_cast<unsigned char>(s.front()))) {
    s.erase(s.begin());
  }
  while (!s.empty() && is_space(static_cast<unsigned char>(s.back()))) {
    s.pop_back();
  }
  return s;
}

bool is_all_digits(const std::string_view text) {
  if (text.empty()) {
    return false;
  }
  for (const char c : text) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  return true;
}

bool path_has_extension(const fs::path& path, const std::string& extension) {
  const std::string file_ext = to_lower(path.extension().string());
  return file_ext == extension;
}

double elapsed_seconds_since(const std::chrono::steady_clock::time_point start) {
  const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  return std::chrono::duration<double>(now - start).count();
}

}  // namespace

MeshFormat parse_mesh_format(const std::string_view value) {
  const std::string normalized = to_lower(value);
  if (normalized == "ply") {
    return MeshFormat::Ply;
  }
  if (normalized == "obj") {
    return MeshFormat::Obj;
  }
  throw std::invalid_argument("Unsupported mesh format: " + std::string(value) +
                              " (expected ply or obj)");
}

std::string_view mesh_format_extension(const MeshFormat format) {
  switch (format) {
    case MeshFormat::Ply:
      return ".ply";
    case MeshFormat::Obj:
      return ".obj";
  }
  return ".ply";
}

std::string_view mesh_format_name(const MeshFormat format) {
  switch (format) {
    case MeshFormat::Ply:
      return "ply";
    case MeshFormat::Obj:
      return "obj";
  }
  return "ply";
}

std::optional<std::uint64_t> mesh_filename_trailing_index(const std::string_view filename) {
  const std::size_t dot = filename.rfind('.');
  const std::string_view stem = dot == std::string_view::npos ? filename : filename.substr(0, dot);
  const std::size_t underscore = stem.rfind('_');
  if (underscore == std::string_view::npos || underscore + 1 >= stem.size()) {
    return std::nullopt;
  }
  const std::string_view index_text = stem.substr(underscore + 1);
  if (!is_all_digits(index_text)) {
    return std::nullopt;
  }
  return std::stoull(std::string(index_text));
}

bool compare_mesh_paths_by_filename(const fs::path& a, const fs::path& b) {
  const std::string name_a = a.filename().string();
  const std::string name_b = b.filename().string();
  const std::optional<std::uint64_t> index_a = mesh_filename_trailing_index(name_a);
  const std::optional<std::uint64_t> index_b = mesh_filename_trailing_index(name_b);
  if (index_a && index_b && *index_a != *index_b) {
    return *index_a < *index_b;
  }
  return name_a < name_b;
}

void sort_mesh_paths_by_filename(std::vector<fs::path>& paths) {
  std::sort(paths.begin(), paths.end(), compare_mesh_paths_by_filename);
}

std::string normalize_mesh_extension(const std::string_view ext) {
  std::string normalized(ext);
  normalized = trim(normalized);
  if (normalized.empty()) {
    throw std::invalid_argument("mesh file extension must not be empty");
  }
  if (normalized.front() != '.') {
    normalized.insert(normalized.begin(), '.');
  }
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return normalized;
}

std::vector<fs::path> list_mesh_files_in_directory(const fs::path& input_dir,
                                                   ListMeshFilesOptions opts) {
  const std::string extension = normalize_mesh_extension(opts.extension);

  std::vector<fs::path> mesh_files;
  for (const auto& entry : fs::directory_iterator(input_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const fs::path p = entry.path();
    if (path_has_extension(p, extension)) {
      mesh_files.push_back(p);
    }
  }
  sort_mesh_paths_by_filename(mesh_files);

  if (mesh_files.empty()) {
    throw std::runtime_error("no " + extension + " files found in " + input_dir.string());
  }

  if (opts.max_objects > 0 && mesh_files.size() > opts.max_objects) {
    mesh_files.resize(opts.max_objects);
  }

  return mesh_files;
}

DatasetMeshListing list_dataset_meshes(const fs::path& input_dir, ListMeshFilesOptions opts) {
  DatasetMeshListing listing;
  listing.input_dir = input_dir;

  if (const std::optional<fs::path> manifest = find_pack_manifest_for_dataset(input_dir)) {
    listing.source = DatasetMeshSource::Pack;
    listing.pack_manifest = *manifest;

    PlyMergeManifest manifest_data = load_ply_merge_manifest(*manifest);
    std::vector<PlyMergeEntry> entries = manifest_data.entries;
    std::sort(entries.begin(), entries.end(),
              [](const PlyMergeEntry& a, const PlyMergeEntry& b) {
                return a.mesh_index < b.mesh_index;
              });

    if (opts.max_objects > 0 && entries.size() > opts.max_objects) {
      entries.resize(opts.max_objects);
    }

    listing.paths.reserve(entries.size());
    for (const PlyMergeEntry& entry : entries) {
      listing.paths.push_back(input_dir / entry.original_filename);
    }

    if (listing.paths.empty()) {
      throw std::runtime_error("no meshes listed in pack manifest: " + manifest->string());
    }

    return listing;
  }

  listing.source = DatasetMeshSource::PerFile;
  listing.paths = list_mesh_files_in_directory(input_dir, opts);
  return listing;
}

TinMesh read_dataset_mesh(const DatasetMeshListing& listing, const std::size_t index) {
  if (index >= listing.paths.size()) {
    throw std::out_of_range("read_dataset_mesh: index out of range");
  }

  if (listing.source == DatasetMeshSource::Pack) {
    return read_ply_from_merge(listing.pack_manifest,
                               listing.paths[index].filename().string());
  }
  return read_ply(listing.paths[index].string());
}

std::vector<MeshVertex> tin_mesh_vertices(const TinMesh& mesh) {
  std::vector<MeshVertex> vertices;
  vertices.reserve(mesh.vertices.size());
  for (const auto& vertex : mesh.vertices) {
    vertices.push_back(vertex);
  }
  return vertices;
}

ListMeshFilesOptions ply_list_options(const std::size_t max_objects) {
  ListMeshFilesOptions opts;
  opts.max_objects = max_objects;
  opts.extension = mesh_format_extension(MeshFormat::Ply);
  return opts;
}

DatasetMeshListing list_dataset_meshes_for_command(const fs::path& input_dir,
                                                   ListMeshFilesOptions opts,
                                                   const std::string_view command) {
  try {
    return list_dataset_meshes(input_dir, opts);
  } catch (const std::runtime_error& error) {
    throw std::runtime_error(std::string(command) + ": " + error.what());
  }
}

void require_non_empty_mesh(const TinMesh& mesh, const std::string_view command,
                            const fs::path& mesh_path) {
  if (mesh.vertices.empty()) {
    throw std::runtime_error(std::string(command) + ": mesh has no vertices: " + mesh_path.string());
  }
}

void print_dataset_mesh_source(std::ostream& out, const DatasetMeshListing& listing) {
  if (listing.source == DatasetMeshSource::Pack) {
    out << "  mesh_source: pack (" << listing.pack_manifest.string() << ")\n";
  } else {
    out << "  mesh_source: per-file\n";
  }
}

LoadedDatasetMeshes load_all_dataset_meshes(const fs::path& input_dir, ListMeshFilesOptions opts,
                                             const std::string_view command,
                                             const std::string_view progress_label) {
  LoadedDatasetMeshes loaded;
  loaded.listing = list_dataset_meshes_for_command(input_dir, opts, command);
  loaded.meshes.reserve(loaded.listing.paths.size());

  FolderMeshLoadProgress progress(loaded.listing.paths.size(), progress_label);
  for (std::size_t i = 0; i < loaded.listing.paths.size(); ++i) {
    TinMesh mesh = read_dataset_mesh(loaded.listing, i);
    require_non_empty_mesh(mesh, command, loaded.listing.paths[i]);
    loaded.meshes.push_back(std::move(mesh));
    progress.mark_loaded(i + 1);
  }

  return loaded;
}

std::size_t current_process_resident_bytes() {
#if defined(__APPLE__)
  mach_task_basic_info info{};
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS) {
    return 0;
  }
  return static_cast<std::size_t>(info.resident_size);
#elif defined(__linux__)
  std::ifstream statm("/proc/self/statm");
  std::size_t total_pages = 0;
  std::size_t resident_pages = 0;
  statm >> total_pages >> resident_pages;
  if (!statm) {
    return 0;
  }
  const long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) {
    return 0;
  }
  return resident_pages * static_cast<std::size_t>(page_size);
#else
  return 0;
#endif
}

void report_folder_mesh_load_progress(const std::size_t loaded, const std::size_t total,
                                      const double elapsed_seconds,
                                      const std::string_view label) {
  const std::size_t resident = current_process_resident_bytes();
  std::cout << std::fixed << std::setprecision(6);
  std::cout << label << ": loaded " << loaded << '/' << total << "  elapsed " << elapsed_seconds
            << " s";
  if (resident > 0) {
    std::cout << std::setprecision(2) << "  memory " << (static_cast<double>(resident) / 1e6)
              << " MB";
  }
  std::cout << '\n' << std::flush;
}

FolderMeshLoadProgress::FolderMeshLoadProgress(const std::size_t total,
                                               const std::string_view label)
    : total_(total), label_(label), start_(std::chrono::steady_clock::now()) {}

void FolderMeshLoadProgress::mark_loaded(const std::size_t loaded) {
  if (total_ == 0 || loaded == 0 || loaded > total_) {
    return;
  }
  const bool at_interval = loaded % kFolderMeshLoadProgressInterval == 0;
  const bool at_end = loaded == total_;
  if (!at_interval && !at_end) {
    return;
  }
  report_folder_mesh_load_progress(loaded, total_, elapsed_seconds_since(start_), label_);
}

}  // namespace tin_gen
