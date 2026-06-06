#include "tin_gen/index_merge.hpp"

#include "tin_gen/cpu_timer.hpp"

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tin_gen {
namespace fs = std::filesystem;

namespace {

constexpr const char* kRsManifestVersion = "tin_test rs_merge_manifest v1";
constexpr const char* kKdManifestVersion = "tin_test kd_merge_manifest v1";

std::string bundle_filename(const std::size_t bundle_index, const char* extension) {
  char name[32];
  std::snprintf(name, sizeof(name), "merged_%03zu%s", bundle_index, extension);
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
    throw std::invalid_argument("index merge manifest path must be a file: " +
                                manifest_path.string());
  }
  return manifest_path;
}

IndexMergeManifest load_index_merge_manifest(const fs::path& manifest_path,
                                             const char* version_token,
                                             const char* default_manifest_name) {
  const fs::path resolved = resolve_manifest_path(manifest_path);
  std::ifstream in(resolved);
  if (!in) {
    throw std::runtime_error("Failed to open index merge manifest: " + resolved.string());
  }

  IndexMergeManifest manifest;
  manifest.manifest_path = resolved;
  bool saw_version = false;
  bool in_table = false;

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      if (line.find(version_token) != std::string::npos) {
        saw_version = true;
      }
      if (line.find("# bundle_file\tmesh_index\tmesh_name\toffset_bytes\tsize_bytes") !=
          std::string::npos) {
        in_table = true;
      }
      continue;
    }

    const std::vector<std::string> fields = split_tab_fields(line);
    if (!in_table) {
      if (fields.size() == 2 && fields[0] == "source_dir") {
        manifest.source_dir = fields[1];
      } else if (fields.size() == 2 && fields[0] == "max_indexes_per_bundle") {
        manifest.max_indexes_per_bundle = static_cast<std::size_t>(std::stoull(fields[1]));
      } else if (fields.size() == 2 && fields[0] == "mesh_count") {
        manifest.mesh_count = static_cast<std::size_t>(std::stoull(fields[1]));
      } else if (fields.size() == 2 && fields[0] == "bundle_count") {
        manifest.bundle_count = static_cast<std::size_t>(std::stoull(fields[1]));
      }
      continue;
    }

    if (fields.size() != 5) {
      throw std::runtime_error("Invalid index merge manifest row: " + line);
    }

    IndexMergeEntry entry;
    entry.bundle_file = fields[0];
    entry.mesh_index = static_cast<std::size_t>(std::stoull(fields[1]));
    entry.mesh_name = fields[2];
    entry.offset_bytes = static_cast<std::uint64_t>(std::stoull(fields[3]));
    entry.size_bytes = static_cast<std::uint64_t>(std::stoull(fields[4]));
    manifest.entries.push_back(std::move(entry));
  }

  if (!saw_version) {
    throw std::runtime_error(std::string("Not an index merge manifest (missing version header): ") +
                             default_manifest_name);
  }
  if (manifest.entries.empty()) {
    throw std::runtime_error("Index merge manifest has no entries: " + resolved.string());
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

void write_index_merge_manifest(const IndexMergeManifest& manifest, const char* version_token) {
  std::ofstream out(manifest.manifest_path);
  if (!out) {
    throw std::runtime_error("Failed to write index merge manifest: " +
                             manifest.manifest_path.string());
  }

  out << "# " << version_token << '\n';
  out << "source_dir\t" << manifest.source_dir.string() << '\n';
  out << "max_indexes_per_bundle\t" << manifest.max_indexes_per_bundle << '\n';
  out << "mesh_count\t" << manifest.mesh_count << '\n';
  out << "bundle_count\t" << manifest.bundle_count << '\n';
  out << "# bundle_file\tmesh_index\tmesh_name\toffset_bytes\tsize_bytes\n";
  for (const auto& entry : manifest.entries) {
    out << entry.bundle_file << '\t' << entry.mesh_index << '\t' << entry.mesh_name << '\t'
        << entry.offset_bytes << '\t' << entry.size_bytes << '\n';
  }
}

std::vector<std::pair<std::uint64_t, std::uint64_t>> read_bundle_toc_offsets(
    const fs::path& bundle_path) {
  std::ifstream in(bundle_path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open index bundle for manifest: " + bundle_path.string());
  }

  char magic[8]{};
  in.read(magic, 8);
  if (!in) {
    throw std::runtime_error("Failed to read index bundle magic: " + bundle_path.string());
  }

  std::uint64_t count = 0;
  in.read(reinterpret_cast<char*>(&count), sizeof(count));
  if (!in) {
    throw std::runtime_error("Failed to read index bundle count: " + bundle_path.string());
  }

  std::vector<std::pair<std::uint64_t, std::uint64_t>> toc;
  toc.reserve(static_cast<std::size_t>(count));
  for (std::uint64_t j = 0; j < count; ++j) {
    std::uint64_t name_len = 0;
    in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
    if (!in) {
      throw std::runtime_error("Failed to read index bundle name length: " + bundle_path.string());
    }
    in.seekg(static_cast<std::streamoff>(name_len), std::ios::cur);
    std::uint64_t offset = 0;
    std::uint64_t size = 0;
    in.read(reinterpret_cast<char*>(&offset), sizeof(offset));
    in.read(reinterpret_cast<char*>(&size), sizeof(size));
    if (!in) {
      throw std::runtime_error("Failed to read index bundle TOC row: " + bundle_path.string());
    }
    toc.push_back({offset, size});
  }
  return toc;
}

void notify_index_bundle_written(const IndexBundleWrittenCallback& callback,
                                 const std::string& bundle_file,
                                 const std::vector<std::size_t>& pending_mesh_indices,
                                 const fs::path& bundle_path, const double write_wall_seconds,
                                 const double write_cpu_seconds) {
  if (!callback || pending_mesh_indices.empty()) {
    return;
  }

  IndexBundleWrittenInfo info;
  info.bundle_file = bundle_file;
  info.index_count = pending_mesh_indices.size();
  info.file_size_bytes = static_cast<std::size_t>(fs::file_size(bundle_path));
  info.first_mesh_index = pending_mesh_indices.front();
  info.last_mesh_index = pending_mesh_indices.back();
  info.write_wall_seconds = write_wall_seconds;
  info.write_cpu_seconds = write_cpu_seconds;
  callback(info);
}

}  // namespace

std::optional<fs::path> find_rs_index_merge_manifest(const fs::path& dir) {
  if (!fs::exists(dir) || !fs::is_directory(dir)) {
    return std::nullopt;
  }
  const fs::path manifest = dir / kRsMergeManifestFilename;
  if (fs::is_regular_file(manifest)) {
    return manifest;
  }
  return std::nullopt;
}

std::optional<fs::path> find_kd_index_merge_manifest(const fs::path& dir) {
  if (!fs::exists(dir) || !fs::is_directory(dir)) {
    return std::nullopt;
  }
  const fs::path manifest = dir / kKdMergeManifestFilename;
  if (fs::is_regular_file(manifest)) {
    return manifest;
  }
  return std::nullopt;
}

IndexMergeManifest load_rs_index_merge_manifest(const fs::path& manifest_path) {
  return load_index_merge_manifest(manifest_path, kRsManifestVersion, kRsMergeManifestFilename);
}

IndexMergeManifest load_kd_index_merge_manifest(const fs::path& manifest_path) {
  return load_index_merge_manifest(manifest_path, kKdManifestVersion, kKdMergeManifestFilename);
}

RsIndexMergeWriter::RsIndexMergeWriter(const fs::path output_dir, const fs::path source_dir,
                                       const std::size_t max_indexes_per_bundle)
    : output_dir_(output_dir),
      source_dir_(source_dir),
      max_indexes_per_bundle_(max_indexes_per_bundle) {
  if (max_indexes_per_bundle_ == 0) {
    throw std::invalid_argument("RsIndexMergeWriter: max_indexes_per_bundle must be > 0");
  }
  fs::create_directories(output_dir_);
}

void RsIndexMergeWriter::set_bundle_written_callback(const IndexBundleWrittenCallback callback) {
  bundle_written_callback_ = std::move(callback);
}

void RsIndexMergeWriter::add(const std::size_t mesh_index, std::string mesh_name, RsTree3d tree) {
  batch_.push_back(RsTreeBundleEntry{std::move(mesh_name), std::move(tree)});
  pending_mesh_indices_.push_back(mesh_index);
  ++mesh_count_;
  if (batch_.size() >= max_indexes_per_bundle_) {
    flush_batch();
  }
}

void RsIndexMergeWriter::flush_batch() {
  if (batch_.empty()) {
    return;
  }

  const std::string bundle_file = bundle_filename(bundle_index_, kRsMergeBundleExtension);
  const fs::path bundle_path = output_dir_ / bundle_file;
  CpuTimer cpu;
  WallTimer wall;
  cpu.start();
  wall.start();
  save_rs_tree_bundle(bundle_path.string(), batch_);
  cpu.stop();
  wall.stop();

  const std::vector<std::pair<std::uint64_t, std::uint64_t>> toc =
      read_bundle_toc_offsets(bundle_path);
  if (toc.size() != batch_.size()) {
    throw std::runtime_error("RS index merge bundle TOC size mismatch: " + bundle_path.string());
  }

  for (std::size_t i = 0; i < batch_.size(); ++i) {
    IndexMergeEntry entry;
    entry.bundle_file = bundle_file;
    entry.mesh_index = pending_mesh_indices_[i];
    entry.mesh_name = batch_[i].name;
    entry.offset_bytes = toc[i].first;
    entry.size_bytes = toc[i].second;
    manifest_entries_.push_back(std::move(entry));
  }

  notify_index_bundle_written(bundle_written_callback_, bundle_file, pending_mesh_indices_,
                              bundle_path, wall.elapsed_seconds(), cpu.elapsed_seconds());

  batch_.clear();
  pending_mesh_indices_.clear();
  ++bundle_index_;
  ++bundle_count_;
}

void RsIndexMergeWriter::finish() {
  flush_batch();

  IndexMergeManifest manifest;
  manifest.source_dir = source_dir_;
  manifest.manifest_path = output_dir_ / kRsMergeManifestFilename;
  manifest.max_indexes_per_bundle = max_indexes_per_bundle_;
  manifest.mesh_count = mesh_count_;
  manifest.bundle_count = bundle_count_;
  manifest.entries = manifest_entries_;
  write_index_merge_manifest(manifest, kRsManifestVersion);
}

KdIndexMergeWriter::KdIndexMergeWriter(const fs::path output_dir, const fs::path source_dir,
                                       const std::size_t max_indexes_per_bundle)
    : output_dir_(output_dir),
      source_dir_(source_dir),
      max_indexes_per_bundle_(max_indexes_per_bundle) {
  if (max_indexes_per_bundle_ == 0) {
    throw std::invalid_argument("KdIndexMergeWriter: max_indexes_per_bundle must be > 0");
  }
  fs::create_directories(output_dir_);
}

void KdIndexMergeWriter::set_bundle_written_callback(const IndexBundleWrittenCallback callback) {
  bundle_written_callback_ = std::move(callback);
}

void KdIndexMergeWriter::add(const std::size_t mesh_index, std::string mesh_name, KdTree3d tree) {
  batch_.push_back(KdTreeBundleEntry{std::move(mesh_name), std::move(tree)});
  pending_mesh_indices_.push_back(mesh_index);
  ++mesh_count_;
  if (batch_.size() >= max_indexes_per_bundle_) {
    flush_batch();
  }
}

void KdIndexMergeWriter::flush_batch() {
  if (batch_.empty()) {
    return;
  }

  const std::string bundle_file = bundle_filename(bundle_index_, kKdMergeBundleExtension);
  const fs::path bundle_path = output_dir_ / bundle_file;
  CpuTimer cpu;
  WallTimer wall;
  cpu.start();
  wall.start();
  save_kd_tree_bundle(bundle_path.string(), batch_);
  cpu.stop();
  wall.stop();

  const std::vector<std::pair<std::uint64_t, std::uint64_t>> toc =
      read_bundle_toc_offsets(bundle_path);
  if (toc.size() != batch_.size()) {
    throw std::runtime_error("KD index merge bundle TOC size mismatch: " + bundle_path.string());
  }

  for (std::size_t i = 0; i < batch_.size(); ++i) {
    IndexMergeEntry entry;
    entry.bundle_file = bundle_file;
    entry.mesh_index = pending_mesh_indices_[i];
    entry.mesh_name = batch_[i].name;
    entry.offset_bytes = toc[i].first;
    entry.size_bytes = toc[i].second;
    manifest_entries_.push_back(std::move(entry));
  }

  notify_index_bundle_written(bundle_written_callback_, bundle_file, pending_mesh_indices_,
                              bundle_path, wall.elapsed_seconds(), cpu.elapsed_seconds());

  batch_.clear();
  pending_mesh_indices_.clear();
  ++bundle_index_;
  ++bundle_count_;
}

void KdIndexMergeWriter::finish() {
  flush_batch();

  IndexMergeManifest manifest;
  manifest.source_dir = source_dir_;
  manifest.manifest_path = output_dir_ / kKdMergeManifestFilename;
  manifest.max_indexes_per_bundle = max_indexes_per_bundle_;
  manifest.mesh_count = mesh_count_;
  manifest.bundle_count = bundle_count_;
  manifest.entries = manifest_entries_;
  write_index_merge_manifest(manifest, kKdManifestVersion);
}

}  // namespace tin_gen
