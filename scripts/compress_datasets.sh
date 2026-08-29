#!/usr/bin/env bash
set -euo pipefail

# Merge (compress) normalized PLY files into pack bundles.
#
# Usage:
#   ./scripts/compress_datasets.sh [small|full]
#
# Input:  ../tin_exp/<dataset>/norm/
# Output: ../tin_exp/<dataset>/norm_pack/

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

echo ">>> compress_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    root:   ${EXP_ROOT}"
echo ""

for dataset in "${datasets[@]}"; do
  echo "--- ${dataset} ---"
  mkdir -p "$(dataset_pack "${dataset}")"
  "${BIN}" compress -i "$(dataset_norm "${dataset}")" -o "$(dataset_pack "${dataset}")"
done

echo ""
echo "Done: ${EXP_ROOT}"
