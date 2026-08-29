#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./scripts/normalize_datasets.sh [small|full]
#
# Synthetic input:  output_synthetic/<preset>/
# Raw input:        ../tin_exp/datasets_raw/<name>/
# Output:           ../tin_exp/<dataset>/norm/

MODE="${1:-small}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=scripts/datasets_common.sh
source "${ROOT}/scripts/datasets_common.sh"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"

echo ">>> normalize_datasets.sh"
echo "    mode:        ${MODE}"
echo "    bin:         ${BIN}"
echo "    synthetic:   ${SYNTH_ROOT}"
echo "    raw:         ${RAW_ROOT}"
echo "    output root: ${EXP_ROOT}"
echo ""

echo "=== Synthetic datasets ==="
if [[ "${MODE}" == "full" ]]; then
  gen_sets=("${SYNTH_GEN_FULL[@]}")
else
  gen_sets=("${SYNTH_GEN_SMALL[@]}")
fi
for gen_name in "${gen_sets[@]}"; do
  dataset="$(synth_gen_to_dataset "${gen_name}")"
  echo "--- ${dataset} ---"
  mkdir -p "$(dataset_norm "${dataset}")"
  "${BIN}" norm -i "${SYNTH_ROOT}/${gen_name}" -o "$(dataset_norm "${dataset}")"
done

echo ""
echo "=== Raw datasets ==="
if [[ "${MODE}" == "full" ]]; then
  raw_maps=("${RAW_NORM_FULL[@]}")
  max_objects_args=()
else
  raw_maps=("${RAW_NORM_SMALL[@]}")
  max_objects_args=(--max-objects 100)
fi
for entry in "${raw_maps[@]}"; do
  raw_name="${entry%%:*}"
  dataset="${entry##*:}"
  echo "--- ${dataset} (from ${raw_name}) ---"
  mkdir -p "$(dataset_norm "${dataset}")"
  "${BIN}" norm -i "${RAW_ROOT}/${raw_name}" -o "$(dataset_norm "${dataset}")" "${max_objects_args[@]}"
done

echo ""
echo "Done: ${EXP_ROOT}"
