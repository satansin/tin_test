#pragma once

#include "tin_gen/mesh_helper.hpp"

#include <cstddef>
#include <vector>

namespace tin_gen {

struct TinMesh;

/// Sample exactly @p num_points uniformly over the surface of a triangular mesh.
///
/// Faces are selected with probability proportional to their area, then each
/// point is sampled uniformly using barycentric coordinates. A seed of zero
/// selects a non-deterministic seed; non-zero seeds make the result repeatable.
[[nodiscard]] std::vector<MeshVertex> sample_points_on_faces(const TinMesh& mesh,
                                                             std::size_t num_points,
                                                             unsigned random_seed = 0);

}  // namespace tin_gen
