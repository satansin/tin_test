#!/usr/bin/env bash
set -euo pipefail

# Compute pairwise distance matrices from packed meshes and prebuilt R*-trees.
# Run compress_datasets.sh and build_rs_datasets.sh first.
#
# Usage:
#   ./scripts/compute_pd_datasets.sh [small|full]
#
# Input:  ../tin_exp/<dataset>/norm_pack/, ../tin_exp/<dataset>/norm_rs/
# Output: ../tin_exp/<dataset>/norm_pd/pairwise_distances_vertex.txt

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

echo ">>> compute_pd_datasets.sh"
echo "    mode:   ${MODE}"
echo "    bin:    ${BIN}"
echo "    root:   ${EXP_ROOT}"
echo ""

for dataset in "${datasets[@]}"; do
  echo "--- ${dataset} ---"
  mkdir -p "${EXP_ROOT}/${dataset}/norm_pd"
  "${BIN}" pd -i "$(dataset_pack "${dataset}")" \
    --rs-dir "$(dataset_rs "${dataset}")" --algorithm vertex \
    -o "$(dataset_pd_file "${dataset}")"
done

echo ""
echo "Done: ${EXP_ROOT}"
