#!/usr/bin/env bash
set -euo pipefail

# Build R*-tree indexes from packed datasets.
# Run compress_datasets.sh first.
#
# Usage:
#   ./scripts/build_rs_datasets.sh [small|full]
#
# Input:  ../tin_exp/<dataset>/norm_pack/
# Output: ../tin_exp/<dataset>/norm_rs/

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

echo ">>> build_rs_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    root:   ${EXP_ROOT}"
echo ""

for dataset in "${datasets[@]}"; do
  echo "--- ${dataset} ---"
  mkdir -p "$(dataset_rs "${dataset}")"
  "${BIN}" rs -i "$(dataset_pack "${dataset}")" -o "$(dataset_rs "${dataset}")" --combined
done

echo ""
echo "Done: ${EXP_ROOT}"
