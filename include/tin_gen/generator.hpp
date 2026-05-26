#pragma once

#include "tin_gen/mesh_format.hpp"
#include "tin_gen/tin_mesh.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace tin_gen {

/// Generate random 3D TINs (TriMesh2 + Qhull convex hull).
/// Each mesh has exactly @p num_vertices_per_object hull vertices.
std::vector<TinMesh> generate_random_tin(std::size_t num_objects,
                                         std::size_t num_vertices_per_object = 50,
                                         double scale = 10.0,
                                         unsigned random_seed = 0);

/// Write meshes as output_dir/object_1.<ext>, ... using the selected format.
void save_objects_as_files(const std::vector<TinMesh>& objects,
                           const std::string& output_dir,
                           MeshFormat format);

}  // namespace tin_gen
