#!/usr/bin/env bash
set -euo pipefail

# Regenerate the full ShapeNetCore dataset and required derived outputs.
# Outputs: norm, norm_pack, norm_rs, norm_ptsample_<N>

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"
DATASET="${ROOT}/../tin_exp/real_ShapeNetCore"
RAW="${ROOT}/../tin_exp/datasets_raw/ShapeNetCore"

rm -rf \
  "${DATASET}/norm" \
  "${DATASET}/norm_pack" \
  "${DATASET}/norm_rs" \
  "${DATASET}"/norm_ptsample_*

mkdir -p "${DATASET}/norm"

"${BIN}" norm \
  --input-dir "${RAW}" \
  --output-dir "${DATASET}/norm"

"${BIN}" validate \
  --input-dir "${DATASET}/norm" \
  --report "${DATASET}/validate_report.tsv"

"${BIN}" compress \
  --input-dir "${DATASET}/norm" \
  --output-dir "${DATASET}/norm_pack"

"${BIN}" rs \
  --input-dir "${DATASET}/norm_pack" \
  --output-dir "${DATASET}/norm_rs" \
  --combined

for num_points in 512 1024 2048; do
  ptsample_dir="${DATASET}/norm_ptsample_${num_points}"
  ptsample_pack_dir="${DATASET}/norm_ptsample_${num_points}_pack"
  "${BIN}" ptsample \
    --input-dir "${DATASET}/norm" \
    --output-dir "${ptsample_dir}" \
    --num-points "${num_points}"
  "${BIN}" compress \
    --input-dir "${ptsample_dir}" \
    --output-dir "${ptsample_pack_dir}"
done

echo "Finished regenerating ${DATASET}"
