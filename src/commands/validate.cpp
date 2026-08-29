#include "tin_gen/commands/validate.hpp"

#include "tin_gen/config.hpp"
#include "tin_gen/face_sampling.hpp"
#include "tin_gen/mesh_helper.hpp"
#include "tin_gen/tin_mesh.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace tin_gen {

int run_validate(const ValidateConfig& config) {
  const std::filesystem::path input_dir(config.input_dir);
  if (!std::filesystem::exists(input_dir) || !std::filesystem::is_directory(input_dir)) {
    throw std::runtime_error("validate: input_dir is not a directory: " + input_dir.string());
  }

  const DatasetMeshListing listing = list_dataset_meshes_for_command(
      input_dir, ply_list_options(config.max_objects), "validate");
  print_dataset_mesh_source(std::cout, listing);

  DatasetMeshLoadProgress progress(listing, "validate mesh files");
  std::vector<std::string> problems;
  problems.reserve(64);

  std::ofstream report_out;
  if (!config.report_path.empty()) {
    const std::filesystem::path report_path(config.report_path);
    if (report_path.has_parent_path()) {
      std::filesystem::create_directories(report_path.parent_path());
    }
    report_out.open(report_path);
    if (!report_out) {
      throw std::runtime_error("validate: failed to open report file: " + config.report_path);
    }
    report_out << "# tin_test validate report\n"
               << "# input_dir\t" << input_dir.string() << '\n'
               << "# mesh_count\t" << listing.paths.size() << '\n'
               << "# columns: mesh\treason\n";
  }

  for (std::size_t i = 0; i < listing.paths.size(); ++i) {
    const std::filesystem::path& mesh_path = listing.paths[i];
    const std::string mesh_name = mesh_path.filename().string();
    try {
      const TinMesh mesh = read_dataset_mesh(listing, i);
      if (const std::optional<std::string> error = mesh_face_sampling_error(mesh)) {
        const std::string line = mesh_name + "\t" + *error;
        problems.push_back(line);
        std::cerr << "INVALID: " << line << '\n';
        if (report_out) {
          report_out << line << '\n';
        }
      }
    } catch (const std::exception& error) {
      const std::string line = mesh_name + "\t" + error.what();
      problems.push_back(line);
      std::cerr << "INVALID: " << line << '\n';
      if (report_out) {
        report_out << line << '\n';
      }
    }
    progress.mark_loaded(i + 1);
  }

  const std::size_t ok_count = listing.paths.size() - problems.size();
  std::cout << "Validated " << listing.paths.size() << " mesh(es) in " << input_dir.string()
            << ": ok=" << ok_count << " invalid=" << problems.size();
  if (!config.report_path.empty()) {
    std::cout << " report=" << config.report_path;
  }
  std::cout << '\n';

  if (!problems.empty()) {
    std::cerr << "validate: found " << problems.size() << " invalid mesh(es)\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

}  // namespace tin_gen
