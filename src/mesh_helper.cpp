#include "tin_gen/mesh_helper.hpp"

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

std::optional<fs::path> resolve_metadata_path(const fs::path& input_dir,
                                              const ListPlyFilesOptions& opts) {
  if (!opts.use_metadata) {
    return std::nullopt;
  }
  if (opts.metadata_path) {
    const fs::path path = *opts.metadata_path;
    if (!fs::exists(path)) {
      throw std::runtime_error("metadata file not found: " + path.string());
    }
    return path;
  }
  const fs::path default_path = input_dir / "metadata.txt";
  if (fs::exists(default_path)) {
    return default_path;
  }
  return std::nullopt;
}

std::vector<std::string> read_metadata_mesh_list(const fs::path& filepath) {
  std::ifstream in(filepath);
  if (!in) {
    throw std::runtime_error("Failed to open metadata file: " + filepath.string());
  }

  std::vector<std::string> names;
  std::string line;
  bool first = true;
  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (first) {
      first = false;
      if (line.find("mesh_name") != std::string::npos) {
        continue;
      }
    }
    std::istringstream iss(line);
    std::string mesh_name;
    if (!std::getline(iss, mesh_name, ',')) {
      continue;
    }
    mesh_name = trim(mesh_name);
    if (!mesh_name.empty()) {
      names.push_back(mesh_name);
    }
  }
  return names;
}

std::vector<fs::path> list_ply_files_in_directory(const fs::path& input_dir,
                                                  ListPlyFilesOptions opts) {
  std::vector<fs::path> ply_files;
  const std::optional<fs::path> metadata_path = resolve_metadata_path(input_dir, opts);

  if (metadata_path) {
    const std::vector<std::string> metadata_order =
        read_metadata_mesh_list(*metadata_path);
    if (metadata_order.empty()) {
      throw std::runtime_error("metadata file had no mesh entries: " +
                               metadata_path->string());
    }
    for (const auto& filename : metadata_order) {
      const fs::path p = input_dir / filename;
      if (fs::exists(p) && fs::is_regular_file(p) && p.extension() == ".ply") {
        ply_files.push_back(p);
      }
    }
  } else {
    for (const auto& entry : fs::directory_iterator(input_dir)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const fs::path p = entry.path();
      if (p.extension() == ".ply") {
        ply_files.push_back(p);
      }
    }
    sort_mesh_paths_by_filename(ply_files);
  }

  if (ply_files.empty()) {
    throw std::runtime_error("no .ply files found in " + input_dir.string());
  }

  if (opts.max_objects > 0 && ply_files.size() > opts.max_objects) {
    ply_files.resize(opts.max_objects);
  }

  return ply_files;
}

namespace {

double elapsed_seconds_since(const std::chrono::steady_clock::time_point start) {
  const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  return std::chrono::duration<double>(now - start).count();
}

}  // namespace

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
  std::cout << '\n';
}

FolderMeshLoadProgress::FolderMeshLoadProgress(const std::size_t total,
                                               const std::string_view label)
    : total_(total), label_(label), start_(std::chrono::steady_clock::now()) {}

void FolderMeshLoadProgress::mark_loaded(const std::size_t loaded) {
  if (total_ == 0 || loaded == 0 || loaded > total_) {
    return;
  }
  const bool at_interval =
      loaded % kFolderMeshLoadProgressInterval == 0;
  const bool at_end = loaded == total_;
  if (!at_interval && !at_end) {
    return;
  }
  report_folder_mesh_load_progress(loaded, total_, elapsed_seconds_since(start_), label_);
}

}  // namespace tin_gen
