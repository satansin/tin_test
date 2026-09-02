#!/usr/bin/env bash
set -euo pipefail

# Sample every normalized mesh at several surface resolutions.
# Writes one point-cloud .ply per mesh. Pack afterward with
# compress_ptsample_datasets.sh.
#
# Usage:
#   ./scripts/ptsample_datasets.sh [small|full]
#
# Input:  ../tin_exp/<dataset>/norm/
# Output: ../tin_exp/<dataset>/norm_ptsample_<N>/

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

echo ">>> ptsample_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    root:   ${EXP_ROOT}"
echo "    points: ${POINT_COUNTS[*]}"
echo ""

for dataset in "${datasets[@]}"; do
  echo "--- ${dataset} ---"
  for num_points in "${POINT_COUNTS[@]}"; do
    output_dir="$(dataset_ptsample "${dataset}" "${num_points}")"
    echo "    ${num_points} points -> ${output_dir}"
    mkdir -p "${output_dir}"
    "${BIN}" ptsample \
      --input-dir "$(dataset_norm "${dataset}")" \
      --output-dir "${output_dir}" \
      --num-points "${num_points}"
  done
done

echo ""
echo "Done: ${EXP_ROOT}"
