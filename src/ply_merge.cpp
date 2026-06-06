#include "tin_gen/ply_merge.hpp"

#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/tin_mesh.hpp"
#include "tin_gen/cpu_timer.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

constexpr const char* kManifestVersion = "tin_test ply_merge_manifest v1";

std::string bundle_filename(const std::size_t bundle_index) {
  char name[32];
  std::snprintf(name, sizeof(name), "merged_%03zu%s", bundle_index, kPlyMergeBundleExtension);
  return name;
}

std::vector<std::string> split_tab_fields(const std::string& line) {
  std::vector<std::string> fields;
  std::size_t start = 0;
  while (start <= line.size()) {
    const std::size_t tab = line.find('\t', start);
    if (tab == std::string::npos) {
      fields.push_back(line.substr(start));
      break;
    }
    fields.push_back(line.substr(start, tab - start));
    start = tab + 1;
  }
  return fields;
}

fs::path resolve_manifest_path(const fs::path& manifest_path) {
  if (manifest_path.empty()) {
    throw std::invalid_argument("manifest path is empty");
  }
  if (fs::is_directory(manifest_path)) {
    return manifest_path / kPlyMergeManifestFilename;
  }
  return manifest_path;
}

std::vector<char> read_bundle_file_bytes(const fs::path& bundle_path) {
  std::ifstream in(bundle_path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open PLY merge bundle: " + bundle_path.string());
  }

  in.seekg(0, std::ios::end);
  const std::streamoff end = in.tellg();
  if (end < 0) {
    throw std::runtime_error("Failed to size PLY merge bundle: " + bundle_path.string());
  }
  in.seekg(0, std::ios::beg);

  std::vector<char> bytes(static_cast<std::size_t>(end));
  in.read(bytes.data(), end);
  if (!in) {
    throw std::runtime_error("Failed to read PLY merge bundle: " + bundle_path.string());
  }
  return bytes;
}

}  // namespace

PlyMergeDatasetReader::PlyMergeDatasetReader(const fs::path& manifest_path,
                                             const std::size_t max_objects) {
  manifest_ = load_ply_merge_manifest(manifest_path);
  entries_ = manifest_.entries;
  std::sort(entries_.begin(), entries_.end(),
            [](const PlyMergeEntry& a, const PlyMergeEntry& b) {
              return a.mesh_index < b.mesh_index;
            });
  if (max_objects > 0 && entries_.size() > max_objects) {
    entries_.resize(max_objects);
  }
  if (entries_.empty()) {
    throw std::runtime_error("PLY merge manifest has no entries: " +
                             manifest_.manifest_path.string());
  }
}

DatasetMeshListing PlyMergeDatasetReader::make_listing(const fs::path& input_dir) const {
  DatasetMeshListing listing;
  listing.source = DatasetMeshSource::Pack;
  listing.input_dir = input_dir;
  listing.pack_manifest = manifest_.manifest_path;
  listing.paths.reserve(entries_.size());
  listing.pack_bundles.reserve(entries_.size());

  std::string last_bundle;
  for (const PlyMergeEntry& entry : entries_) {
    listing.paths.push_back(input_dir / entry.original_filename);
    listing.pack_bundles.push_back(entry.bundle_file);
    if (entry.bundle_file != last_bundle) {
      listing.pack_bundle_names.push_back(entry.bundle_file);
      last_bundle = entry.bundle_file;
    }
  }
  return listing;
}

void PlyMergeDatasetReader::set_bundle_loaded_callback(const PackBundleLoadedCallback callback) {
  bundle_loaded_callback_ = std::move(callback);
}

void PlyMergeDatasetReader::report_bundle_loaded(const PackBundleLoadedInfo& info) {
  if (bundle_loaded_callback_) {
    bundle_loaded_callback_(info);
    return;
  }

  const double bytes = static_cast<double>(info.size_bytes);
  std::cout << std::fixed;
  std::cout << "pack: loaded bundle " << info.bundle_file << " into memory (";
  if (bytes >= 1e9) {
    std::cout << std::setprecision(2) << (bytes / 1e9) << " GB";
  } else {
    std::cout << std::setprecision(2) << (bytes / 1e6) << " MB";
  }
  std::cout << std::setprecision(6) << ")  bundle read ";
  append_formatted_elapsed_seconds(std::cout, info.read_wall_seconds);
  std::cout << "  CPU ";
  append_formatted_elapsed_seconds(std::cout, info.read_cpu_seconds);
  std::cout << '\n'
            << std::flush;
}

void PlyMergeDatasetReader::ensure_bundle_loaded(const std::string& bundle_file,
                                                 const std::size_t mesh_index) {
  if (loaded_bundle_file_ == bundle_file) {
    return;
  }

  const fs::path bundle_path = manifest_.manifest_path.parent_path() / bundle_file;
  CpuTimer cpu;
  WallTimer wall;
  cpu.start();
  wall.start();
  loaded_bundle_bytes_ = read_bundle_file_bytes(bundle_path);
  cpu.stop();
  wall.stop();
  loaded_bundle_file_ = bundle_file;

  report_bundle_loaded(PackBundleLoadedInfo{
      .bundle_file = bundle_file,
      .size_bytes = loaded_bundle_bytes_.size(),
      .read_wall_seconds = wall.elapsed_seconds(),
      .read_cpu_seconds = cpu.elapsed_seconds(),
      .mesh_index = mesh_index,
  });
}

TinMesh PlyMergeDatasetReader::read_mesh_from_entry(const PlyMergeEntry& entry,
                                                    const std::size_t mesh_index,
                                                    const PlyReadContent content) {
  ensure_bundle_loaded(entry.bundle_file, mesh_index);

  const std::uint64_t end_bytes = entry.offset_bytes + entry.size_bytes;
  if (end_bytes > loaded_bundle_bytes_.size()) {
    throw std::runtime_error("PLY merge entry exceeds bundle size: " + entry.bundle_file + "[" +
                             entry.original_filename + "]");
  }

  const char* const start = loaded_bundle_bytes_.data() + entry.offset_bytes;
  const std::string context = entry.bundle_file + "[" + entry.original_filename + "]";
  return read_ply_memory(start, static_cast<std::size_t>(entry.size_bytes), context, content);
}

TinMesh PlyMergeDatasetReader::read_mesh(const std::size_t index, const PlyReadContent content) {
  if (index >= entries_.size()) {
    throw std::out_of_range("PlyMergeDatasetReader::read_mesh: index out of range");
  }
  return read_mesh_from_entry(entries_[index], index, content);
}

void PlyMergeDatasetReader::release_loaded_bundle() {
  loaded_bundle_file_.clear();
  loaded_bundle_bytes_.clear();
  loaded_bundle_bytes_.shrink_to_fit();
}

namespace {

std::uint64_t file_size_bytes(const fs::path& path) {
  std::error_code ec;
  const auto size = fs::file_size(path, ec);
  if (ec) {
    throw std::runtime_error("Failed to stat PLY file: " + path.string());
  }
  return static_cast<std::uint64_t>(size);
}

void copy_ply_into_bundle(std::ofstream& out, const fs::path& ply_path) {
  std::ifstream in(ply_path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open PLY file for merge: " + ply_path.string());
  }
  out << in.rdbuf();
  if (!out) {
    throw std::runtime_error("Failed to write PLY into merge bundle");
  }
}

std::string trim_line(std::string s) {
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
  while (!s.empty() && is_space(static_cast<unsigned char>(s.front()))) {
    s.erase(s.begin());
  }
  while (!s.empty() && is_space(static_cast<unsigned char>(s.back()))) {
    s.pop_back();
  }
  return s;
}

std::optional<fs::path> manifest_path_if_exists(const fs::path& pack_dir) {
  const fs::path manifest = pack_dir / kPlyMergeManifestFilename;
  std::error_code ec;
  if (fs::is_regular_file(manifest, ec)) {
    return manifest;
  }
  return std::nullopt;
}

std::optional<fs::path> manifest_from_pack_setting(const fs::path& input_dir) {
  const fs::path setting_path = input_dir / kPackSettingFilename;
  std::error_code ec;
  if (!fs::is_regular_file(setting_path, ec)) {
    return std::nullopt;
  }

  std::ifstream in(setting_path);
  if (!in) {
    return std::nullopt;
  }

  std::string line;
  while (std::getline(in, line)) {
    line = trim_line(std::move(line));
    if (line.empty() || line[0] == '#') {
      continue;
    }

    fs::path pack_dir(line);
    if (pack_dir.is_relative()) {
      pack_dir = input_dir / pack_dir;
    }
    pack_dir = fs::weakly_canonical(pack_dir, ec);
    if (ec) {
      pack_dir = (input_dir / line).lexically_normal();
    }
    return manifest_path_if_exists(pack_dir);
  }

  return std::nullopt;
}

std::optional<fs::path> manifest_from_norm_pack_sibling(const fs::path& input_dir) {
  std::vector<fs::path> parts;
  parts.reserve(static_cast<std::size_t>(std::distance(input_dir.begin(), input_dir.end())));
  for (const auto& part : input_dir) {
    parts.push_back(part);
  }

  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (parts[i] != kDatasetsNormDirectoryName) {
      continue;
    }
    parts[i] = kDatasetsNormPackDirectoryName;
    fs::path candidate;
    for (const auto& part : parts) {
      candidate /= part;
    }
    if (const auto manifest = manifest_path_if_exists(candidate)) {
      return manifest;
    }
  }

  return std::nullopt;
}

}  // namespace

PlyMergeManifest load_ply_merge_manifest(const fs::path& manifest_path) {
  const fs::path resolved = resolve_manifest_path(manifest_path);
  std::ifstream in(resolved);
  if (!in) {
    throw std::runtime_error("Failed to open PLY merge manifest: " + resolved.string());
  }

  PlyMergeManifest manifest;
  manifest.manifest_path = resolved;
  bool saw_version = false;
  bool in_table = false;

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      if (line.find(kManifestVersion) != std::string::npos) {
        saw_version = true;
      }
      if (line.find("# bundle_file\tmesh_index\toriginal_filename\toffset_bytes\tsize_bytes") !=
          std::string::npos) {
        in_table = true;
      }
      continue;
    }

    const std::vector<std::string> fields = split_tab_fields(line);
    if (!in_table) {
      if (fields.size() == 2 && fields[0] == "source_dir") {
        manifest.source_dir = fields[1];
      } else if (fields.size() == 2 && fields[0] == "max_meshes_per_bundle") {
        manifest.max_meshes_per_bundle = static_cast<std::size_t>(std::stoull(fields[1]));
      } else if (fields.size() == 2 && fields[0] == "mesh_count") {
        manifest.mesh_count = static_cast<std::size_t>(std::stoull(fields[1]));
      } else if (fields.size() == 2 && fields[0] == "bundle_count") {
        manifest.bundle_count = static_cast<std::size_t>(std::stoull(fields[1]));
      }
      continue;
    }

    if (fields.size() != 5) {
      throw std::runtime_error("Invalid manifest table row: " + line);
    }

    PlyMergeEntry entry;
    entry.bundle_file = fields[0];
    entry.mesh_index = static_cast<std::size_t>(std::stoull(fields[1]));
    entry.original_filename = fields[2];
    entry.offset_bytes = static_cast<std::uint64_t>(std::stoull(fields[3]));
    entry.size_bytes = static_cast<std::uint64_t>(std::stoull(fields[4]));
    manifest.entries.push_back(std::move(entry));
  }

  if (!saw_version) {
    throw std::runtime_error("Not a ply merge manifest (missing version header): " +
                             resolved.string());
  }
  if (manifest.entries.empty()) {
    throw std::runtime_error("PLY merge manifest has no entries: " + resolved.string());
  }
  if (manifest.mesh_count == 0) {
    manifest.mesh_count = manifest.entries.size();
  }
  if (manifest.bundle_count == 0) {
    std::unordered_set<std::string> bundles;
    for (const auto& entry : manifest.entries) {
      bundles.insert(entry.bundle_file);
    }
    manifest.bundle_count = bundles.size();
  }

  return manifest;
}

PlyMergeResult write_ply_merge(const fs::path& source_dir, const fs::path& output_dir,
                               PlyMergeOptions opts) {
  if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
    throw std::runtime_error("write_ply_merge: source_dir is not a directory: " +
                             source_dir.string());
  }
  if (opts.max_meshes_per_bundle == 0) {
    throw std::invalid_argument("write_ply_merge: max_meshes_per_bundle must be > 0");
  }

  fs::create_directories(output_dir);

  const std::vector<fs::path> ply_files =
      list_mesh_files_in_directory(source_dir, ply_list_options(opts.max_objects));
  if (ply_files.empty()) {
    throw std::runtime_error("write_ply_merge: no PLY files in " + source_dir.string());
  }

  PlyMergeResult result;
  result.mesh_count = ply_files.size();
  result.manifest_path = output_dir / kPlyMergeManifestFilename;

  std::vector<PlyMergeEntry> entries;
  entries.reserve(ply_files.size());

  std::size_t bundle_index = 0;
  std::size_t meshes_in_bundle = 0;
  std::string current_bundle;
  fs::path current_bundle_path;
  std::ofstream bundle_out;
  std::uint64_t bundle_offset = 0;

  for (std::size_t mesh_index = 0; mesh_index < ply_files.size(); ++mesh_index) {
    if (meshes_in_bundle == 0) {
      if (bundle_out.is_open()) {
        bundle_out.close();
      }
      current_bundle = bundle_filename(bundle_index);
      current_bundle_path = output_dir / current_bundle;
      bundle_out.open(current_bundle_path, std::ios::binary | std::ios::trunc);
      if (!bundle_out) {
        throw std::runtime_error("Failed to create merge bundle: " + current_bundle_path.string());
      }
      bundle_offset = 0;
    }

    const fs::path& ply_path = ply_files[mesh_index];
    const std::uint64_t offset = bundle_offset;
    const std::uint64_t size = file_size_bytes(ply_path);
    copy_ply_into_bundle(bundle_out, ply_path);

    PlyMergeEntry entry;
    entry.bundle_file = current_bundle;
    entry.mesh_index = mesh_index;
    entry.original_filename = ply_path.filename().string();
    entry.offset_bytes = offset;
    entry.size_bytes = size;
    entries.push_back(entry);

    bundle_offset += size;
    ++meshes_in_bundle;

    if (meshes_in_bundle >= opts.max_meshes_per_bundle) {
      ++bundle_index;
      meshes_in_bundle = 0;
    }
  }

  if (bundle_out.is_open()) {
    bundle_out.close();
  }

  result.bundle_count = bundle_index + 1;

  std::ofstream manifest(result.manifest_path);
  if (!manifest) {
    throw std::runtime_error("Failed to write PLY merge manifest: " + result.manifest_path.string());
  }

  manifest << "# " << kManifestVersion << '\n';
  manifest << "source_dir\t" << source_dir.string() << '\n';
  manifest << "max_meshes_per_bundle\t" << opts.max_meshes_per_bundle << '\n';
  manifest << "mesh_count\t" << result.mesh_count << '\n';
  manifest << "bundle_count\t" << result.bundle_count << '\n';
  manifest << "# bundle_file\tmesh_index\toriginal_filename\toffset_bytes\tsize_bytes\n";
  for (const auto& entry : entries) {
    manifest << entry.bundle_file << '\t' << entry.mesh_index << '\t' << entry.original_filename
             << '\t' << entry.offset_bytes << '\t' << entry.size_bytes << '\n';
  }

  return result;
}

TinMesh read_ply_from_merge(const fs::path& manifest_path, const std::string_view original_filename) {
  PlyMergeDatasetReader reader(manifest_path);
  for (std::size_t i = 0; i < reader.mesh_count(); ++i) {
    if (reader.entries()[i].original_filename == original_filename) {
      return reader.read_mesh(i);
    }
  }
  throw std::runtime_error("PLY merge manifest missing mesh: " + std::string(original_filename));
}

TinMesh read_ply_from_merge_by_index(const fs::path& manifest_path, const std::size_t mesh_index) {
  PlyMergeDatasetReader reader(manifest_path);
  for (std::size_t i = 0; i < reader.mesh_count(); ++i) {
    if (reader.entries()[i].mesh_index == mesh_index) {
      return reader.read_mesh(i);
    }
  }
  throw std::runtime_error("PLY merge manifest missing mesh index: " + std::to_string(mesh_index));
}

std::optional<fs::path> find_pack_manifest_for_dataset(const fs::path& input_dir) {
  if (const auto manifest = manifest_from_pack_setting(input_dir)) {
    return manifest;
  }
  if (const auto manifest = manifest_path_if_exists(input_dir)) {
    return manifest;
  }
  return manifest_from_norm_pack_sibling(input_dir);
}

}  // namespace tin_gen
