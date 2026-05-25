#include "tin_gen/generator.hpp"

#include <filesystem>
#include <iostream>

namespace tin_gen {

void save_objects_as_files(const std::vector<TinMesh>& objects,
                           const std::string& output_dir,
                           const MeshFormat format) {
  std::filesystem::create_directories(output_dir);
  const std::string ext(mesh_format_extension(format));

  for (std::size_t i = 0; i < objects.size(); ++i) {
    const std::filesystem::path filepath = std::filesystem::path(output_dir) /
                                           ("object_" + std::to_string(i + 1) + ext);
    write_mesh(filepath.string(), objects[i], format);
    std::cout << "Saved: " << filepath.string() << '\n';
  }
}

}  // namespace tin_gen
