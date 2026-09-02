#!/usr/bin/env bash
set -euo pipefail

# Temporary: ptsample at 4096 directly into pack + R*-tree indexes.
# Skips loose norm_ptsample_<N>/ folders (uses ptsample --pack).
# Same resolution as the official ptsample pipeline (4096 points per mesh).
#
# Usage:
#   ./scripts/tmp_more_ptsamples.sh [small|full]
#
# Input:  ../tin_exp/<dataset>/norm/
# Output: ../tin_exp/<dataset>/norm_ptsample4096_pack/
#         ../tin_exp/<dataset>/norm_ptsample4096_rs/

MODE="${1:-small}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=scripts/datasets_common.sh
source "${ROOT}/scripts/datasets_common.sh"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"

POINT_COUNTS=(4096)

if [[ "${MODE}" == "full" ]]; then
  datasets=("${DATASETS_FULL[@]}")
else
  datasets=("${DATASETS_SMALL[@]}")
fi

echo ">>> tmp_more_ptsamples.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    root:   ${EXP_ROOT}"
echo "    points: ${POINT_COUNTS[*]}"
echo ""

for dataset in "${datasets[@]}"; do
  echo "=== ${dataset} ==="
  norm_dir="$(dataset_norm "${dataset}")"
  if [[ ! -d "${norm_dir}" ]]; then
    echo "    skip: missing norm ${norm_dir}"
    continue
  fi

  for num_points in "${POINT_COUNTS[@]}"; do
    pack_dir="$(dataset_ptsample_pack "${dataset}" "${num_points}")"
    rs_dir="$(dataset_ptsample_rs "${dataset}" "${num_points}")"

    echo "--- ${num_points} points ---"
    echo "    ptsample --pack: ${norm_dir} -> ${pack_dir}"
    mkdir -p "${pack_dir}"
    "${BIN}" ptsample \
      --input-dir "${norm_dir}" \
      --output-dir "${pack_dir}" \
      --num-points "${num_points}" \
      --pack

    echo "    rs:              ${pack_dir} -> ${rs_dir}"
    mkdir -p "${rs_dir}"
    "${BIN}" rs \
      --input-dir "${pack_dir}" \
      --output-dir "${rs_dir}" \
      --combined
  done
done

echo ""
echo "Done: ${EXP_ROOT}"
