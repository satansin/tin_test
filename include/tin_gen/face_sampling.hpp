#pragma once

#include "tin_gen/mesh_helper.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace tin_gen {

struct TinMesh;

/// If @p mesh cannot be used for area-weighted face sampling, returns a short
/// reason; otherwise nullopt. Checks empty geometry, out-of-range face indices,
/// and whether at least one face has positive area.
[[nodiscard]] std::optional<std::string> mesh_face_sampling_error(const TinMesh& mesh);

/// Sample exactly @p num_points uniformly over the surface of a triangular mesh.
///
/// Faces are selected with probability proportional to their area, then each
/// point is sampled uniformly using barycentric coordinates. A seed of zero
/// selects a non-deterministic seed; non-zero seeds make the result repeatable.
[[nodiscard]] std::vector<MeshVertex> sample_points_on_faces(const TinMesh& mesh,
                                                             std::size_t num_points,
                                                             unsigned random_seed = 0);

}  // namespace tin_gen
