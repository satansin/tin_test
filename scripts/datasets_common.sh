# Shared paths and dataset lists for tin_exp pipeline scripts.
# Source from other scripts: source "$(dirname "${BASH_SOURCE[0]}")/datasets_common.sh"
#
# Layout (per dataset under ../tin_exp/<dataset>/):
#   norm/       normalized .ply
#   norm_pack/  compressed bundles + ply_merge_manifest.txt
#   norm_rs/    R*-tree indexes (rs_merge_manifest.txt, merged_*.tinrs)
#   norm_kd/    KD-tree indexes (optional)
#   norm_ptsample_<N>/     sampled point-cloud .ply files (N in {512,1024,2048,4096})
#   norm_ptsample<N>_pack/ packed sampled bundles + ply_merge_manifest.txt
#   norm_ptsample<N>_rs/   R*-tree indexes for sampled packs
#   norm_pd/    pairwise distance matrix output
#
# datasets_raw/ holds upstream PLY/OBJ sources only.
# real_* (without First100) are full server datasets; real_First100_* are local small subsets.

EXP_ROOT="${ROOT}/../tin_exp"
RAW_ROOT="${EXP_ROOT}/datasets_raw"
SYNTH_ROOT="${ROOT}/output_synthetic"

dataset_norm() { echo "${EXP_ROOT}/$1/norm"; }
dataset_pack() { echo "${EXP_ROOT}/$1/norm_pack"; }
dataset_rs() { echo "${EXP_ROOT}/$1/norm_rs"; }
dataset_kd() { echo "${EXP_ROOT}/$1/norm_kd"; }
dataset_ptsample() { echo "${EXP_ROOT}/$1/norm_ptsample_$2"; }
dataset_ptsample_pack() { echo "${EXP_ROOT}/$1/norm_ptsample$2_pack"; }
dataset_ptsample_rs() { echo "${EXP_ROOT}/$1/norm_ptsample$2_rs"; }
dataset_pd_file() { echo "${EXP_ROOT}/$1/norm_pd/pairwise_distances_vertex.txt"; }
dataset_pd_chamfer_ptsample_file() {
  echo "${EXP_ROOT}/$1/norm_pd/pairwise_distances_chamfer_ptsample$2.txt"
}

synth_gen_to_dataset() { echo "synthetic_$1"; }

# Generation folders under output_synthetic/ (no synthetic_ prefix).
SYNTH_GEN_SMALL=(objects100_vertices200 objects100_vertices500 objects1000_vertices200)
SYNTH_GEN_FULL=(
  objects100_vertices200
  objects100_vertices500
  objects1000_vertices200
  objects1000_vertices500
  objects10000_vertices200
  objects10000_vertices500
)

# Dataset folder names (tin_exp/<name>/).
DATASETS_SMALL=(
  synthetic_objects100_vertices200
  synthetic_objects100_vertices500
  synthetic_objects1000_vertices200
  real_First100_ModelNet40
  real_First100_ModelNet40_auto_aligned
  real_First100_ModelNet40_manually_aligned
  real_First100_ShapeNetCore
)

DATASETS_FULL=(
  synthetic_objects100_vertices200
  synthetic_objects100_vertices500
  synthetic_objects1000_vertices200
  synthetic_objects1000_vertices500
  synthetic_objects10000_vertices200
  synthetic_objects10000_vertices500
  real_ModelNet40
  real_ModelNet40_auto_aligned
  real_ModelNet40_manually_aligned
  real_ShapeNetCore
)

# raw_source_dir:output_dataset_dir (normalize only)
RAW_NORM_SMALL=(
  ModelNet40:real_First100_ModelNet40
  ModelNet40_auto_aligned:real_First100_ModelNet40_auto_aligned
  ModelNet40_manually_aligned:real_First100_ModelNet40_manually_aligned
  ShapeNetCore:real_First100_ShapeNetCore
)

RAW_NORM_FULL=(
  ModelNet40:real_ModelNet40
  ModelNet40_auto_aligned:real_ModelNet40_auto_aligned
  ModelNet40_manually_aligned:real_ModelNet40_manually_aligned
  ShapeNetCore:real_ShapeNetCore
)
