#!/usr/bin/env bash
set -euo pipefail

# Regenerate real_First100_ShapeNetCore and all derived outputs.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"
DATASET="${ROOT}/../tin_exp/real_First100_ShapeNetCore"
RAW="${ROOT}/../tin_exp/datasets_raw/ShapeNetCore"

rm -rf \
  "${DATASET}/norm" \
  "${DATASET}/norm_pack" \
  "${DATASET}/norm_rs" \
  "${DATASET}/norm_kd" \
  "${DATASET}/norm_pd" \
  "${DATASET}"/norm_ptsample_*

mkdir -p "${DATASET}/norm"

"${BIN}" norm \
  --input-dir "${RAW}" \
  --output-dir "${DATASET}/norm" \
  --max-objects 100

"${BIN}" compress \
  --input-dir "${DATASET}/norm" \
  --output-dir "${DATASET}/norm_pack"

"${BIN}" rs \
  --input-dir "${DATASET}/norm_pack" \
  --output-dir "${DATASET}/norm_rs" \
  --combined

"${BIN}" kd \
  --input-dir "${DATASET}/norm_pack" \
  --output-dir "${DATASET}/norm_kd" \
  --combined

for num_points in 512 1024 2048; do
  "${BIN}" ptsample \
    --input-dir "${DATASET}/norm" \
    --output-dir "${DATASET}/norm_ptsample_${num_points}" \
    --num-points "${num_points}" \
    --pack
done

mkdir -p "${DATASET}/norm_pd"

"${BIN}" pd \
  --input-dir "${DATASET}/norm_pack" \
  --rs-dir "${DATASET}/norm_rs" \
  --algorithm vertex \
  --output "${DATASET}/norm_pd/pairwise_distances_vertex.txt"

echo "Finished regenerating ${DATASET}"
