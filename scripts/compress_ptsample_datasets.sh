#!/usr/bin/env bash
set -euo pipefail

# Pack sampled point-cloud PLY files into .tinply bundles.
# Run ptsample_datasets.sh first (without --pack).
#
# Usage:
#   ./scripts/compress_ptsample_datasets.sh [small|full]
#
# Input:  ../tin_exp/<dataset>/norm_ptsample_<N>/
# Output: ../tin_exp/<dataset>/norm_ptsample_<N>_pack/

MODE="${1:-small}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=scripts/datasets_common.sh
source "${ROOT}/scripts/datasets_common.sh"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"

POINT_COUNTS=(512 1024 2048)

if [[ "${MODE}" == "full" ]]; then
  datasets=("${DATASETS_FULL[@]}")
else
  datasets=("${DATASETS_SMALL[@]}")
fi

echo ">>> compress_ptsample_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    root:   ${EXP_ROOT}"
echo "    points: ${POINT_COUNTS[*]}"
echo ""

for dataset in "${datasets[@]}"; do
  echo "--- ${dataset} ---"
  for num_points in "${POINT_COUNTS[@]}"; do
    input_dir="$(dataset_ptsample "${dataset}" "${num_points}")"
    output_dir="$(dataset_ptsample_pack "${dataset}" "${num_points}")"
    echo "    ${num_points} points: ${input_dir} -> ${output_dir}"
    mkdir -p "${output_dir}"
    "${BIN}" compress \
      --input-dir "${input_dir}" \
      --output-dir "${output_dir}"
  done
done

echo ""
echo "Done: ${EXP_ROOT}"
