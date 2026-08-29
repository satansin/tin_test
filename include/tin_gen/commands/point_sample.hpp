#pragma once

namespace tin_gen {

struct PointSampleConfig;

/// Sample points uniformly over the faces of one input TIN.
int run_point_sample(const PointSampleConfig& config);

}  // namespace tin_gen
