#pragma once

namespace tin_gen {

struct ValidateConfig;

/// Check every mesh in a dataset folder for load / topology problems that break
/// face sampling and related pipeline steps. Continues after failures and
/// reports all bad meshes. Returns EXIT_FAILURE if any mesh is invalid.
int run_validate(const ValidateConfig& config);

}  // namespace tin_gen
