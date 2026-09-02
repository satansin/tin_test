#!/usr/bin/env bash
set -euo pipefail

# Build R*-tree indexes for packed ptsample datasets.
# Run compress_ptsample_datasets.sh (or rename packed ptsample folders) first.
#
# Usage:
#   ./scripts/build_rs_ptsample_datasets.sh [small|full]
#
# Input:  ../tin_exp/<dataset>/norm_ptsample<N>_pack/
# Output: ../tin_exp/<dataset>/norm_ptsample<N>_rs/

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

echo ">>> build_rs_ptsample_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    root:   ${EXP_ROOT}"
echo "    points: ${POINT_COUNTS[*]}"
echo ""

for dataset in "${datasets[@]}"; do
  echo "--- ${dataset} ---"
  for num_points in "${POINT_COUNTS[@]}"; do
    input_dir="$(dataset_ptsample_pack "${dataset}" "${num_points}")"
    output_dir="$(dataset_ptsample_rs "${dataset}" "${num_points}")"
    echo "    ${num_points} points: ${input_dir} -> ${output_dir}"
    if [[ ! -d "${input_dir}" ]]; then
      echo "    skip: missing input ${input_dir}"
      continue
    fi
    mkdir -p "${output_dir}"
    "${BIN}" rs \
      --input-dir "${input_dir}" \
      --output-dir "${output_dir}" \
      --combined
  done
done

echo ""
echo "Done: ${EXP_ROOT}"
