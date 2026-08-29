#!/usr/bin/env bash
set -euo pipefail

# Build KD-tree indexes from packed datasets (optional; not used by the current PD pipeline).
# Run compress_datasets.sh first.
#
# Usage:
#   ./scripts/build_kd_datasets.sh [small|full]
#
# Input:  ../tin_exp/<dataset>/norm_pack/
# Output: ../tin_exp/<dataset>/norm_kd/

MODE="${1:-small}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=scripts/datasets_common.sh
source "${ROOT}/scripts/datasets_common.sh"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"

if [[ "${MODE}" == "full" ]]; then
  datasets=("${DATASETS_FULL[@]}")
else
  datasets=("${DATASETS_SMALL[@]}")
fi

echo ">>> build_kd_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    root:   ${EXP_ROOT}"
echo "    note:   KD indexes are optional; PD uses norm_rs by default"
echo ""

for dataset in "${datasets[@]}"; do
  echo "--- ${dataset} ---"
  mkdir -p "$(dataset_kd "${dataset}")"
  "${BIN}" kd -i "$(dataset_pack "${dataset}")" -o "$(dataset_kd "${dataset}")" --combined
done

echo ""
echo "Done: ${EXP_ROOT}"
