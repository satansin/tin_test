#!/usr/bin/env bash
set -euo pipefail

# Compute Chamfer pairwise distance matrices from packed ptsamples and R*-trees.
# Run ptsample_datasets.sh → compress_ptsample_datasets.sh → build_rs_ptsample_datasets.sh first.
#
# Usage:
#   ./scripts/compute_pd_ptsample_datasets.sh [small|full]
#
# Input:  ../tin_exp/<dataset>/norm_ptsample<N>_pack/, ../tin_exp/<dataset>/norm_ptsample<N>_rs/
# Output: ../tin_exp/<dataset>/norm_pd/pairwise_distances_chamfer_ptsample<N>.txt

MODE="${1:-small}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=scripts/datasets_common.sh
source "${ROOT}/scripts/datasets_common.sh"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"

POINT_COUNTS=(512 1024 2048 4096)

if [[ "${MODE}" == "full" ]]; then
  datasets=("${DATASETS_FULL[@]}")
else
  datasets=("${DATASETS_SMALL[@]}")
fi

echo ">>> compute_pd_ptsample_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    root:   ${EXP_ROOT}"
echo "    points: ${POINT_COUNTS[*]}"
echo ""

for dataset in "${datasets[@]}"; do
  echo "--- ${dataset} ---"
  mkdir -p "${EXP_ROOT}/${dataset}/norm_pd"
  for num_points in "${POINT_COUNTS[@]}"; do
    input_dir="$(dataset_ptsample_pack "${dataset}" "${num_points}")"
    rs_dir="$(dataset_ptsample_rs "${dataset}" "${num_points}")"
    output_file="$(dataset_pd_chamfer_ptsample_file "${dataset}" "${num_points}")"
    echo "    ${num_points} points: ${input_dir} + ${rs_dir} -> ${output_file}"
    if [[ ! -d "${input_dir}" ]]; then
      echo "    skip: missing pack ${input_dir}"
      continue
    fi
    if [[ ! -d "${rs_dir}" ]]; then
      echo "    skip: missing rs ${rs_dir}"
      continue
    fi
    "${BIN}" pd -i "${input_dir}" \
      --rs-dir "${rs_dir}" --algorithm chamfer \
      -o "${output_file}"
  done
done

echo ""
echo "Done: ${EXP_ROOT}"
