#!/usr/bin/env bash
set -euo pipefail

# Run the local sample pipeline using the sample_* folders.
#
# Usage:
#   ./scripts/run_samples.sh
#
# Environment:
#   TIN_TEST_BIN  Path to tin_test (default: build/release/tin_test)

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${TIN_TEST_BIN:-${ROOT}/build/release/tin_test}"
if [[ ! -x "${BIN}" && -x "${ROOT}/build/debug/tin_test" ]]; then
  BIN="${ROOT}/build/debug/tin_test"
fi

SAMPLE_GENERATED_DIR="${ROOT}/sample_gen"
SAMPLE_NORMALIZED_DIR="${ROOT}/sample_normalized"
SAMPLE_POINTS_DIR="${ROOT}/sample_points"
SAMPLE_PACK_DIR="${ROOT}/sample_pack"
SAMPLE_RS_DIR="${ROOT}/sample_rsvertices"
SAMPLE_KD_DIR="${ROOT}/sample_kdvertices"
SAMPLE_PD_PATH="${ROOT}/sample_pd/pairwise_distances_vertex.txt"

if [[ ! -x "${BIN}" ]]; then
  echo "error: tin_test not found at ${BIN}" >&2
  echo "Build with:" >&2
  echo "  cmake --preset release && cmake --build --preset release" >&2
  exit 1
fi

echo ">>> run_samples.sh"
echo "    bin: ${BIN}"
echo ""

"${BIN}" generate --output-dir "${SAMPLE_GENERATED_DIR}"
"${BIN}" normalize --input-dir "${SAMPLE_GENERATED_DIR}" \
  --output-dir "${SAMPLE_NORMALIZED_DIR}"
"${BIN}" ptsample --input-dir "${SAMPLE_NORMALIZED_DIR}" \
  --output-dir "${SAMPLE_POINTS_DIR}" --num-points 10000 --seed 42
"${BIN}" compress --input-dir "${SAMPLE_NORMALIZED_DIR}" \
  --output-dir "${SAMPLE_PACK_DIR}"
"${BIN}" rs --input-dir "${SAMPLE_NORMALIZED_DIR}" \
  --output-dir "${SAMPLE_RS_DIR}"
"${BIN}" kd --input-dir "${SAMPLE_NORMALIZED_DIR}" \
  --output-dir "${SAMPLE_KD_DIR}"
"${BIN}" pairwise_distance --input-dir "${SAMPLE_NORMALIZED_DIR}" \
  --rs-dir "${SAMPLE_RS_DIR}" --output "${SAMPLE_PD_PATH}"

echo ""
echo "Sample outputs:"
echo "  generated:   ${SAMPLE_GENERATED_DIR}"
echo "  normalized:  ${SAMPLE_NORMALIZED_DIR}"
echo "  point sample: ${SAMPLE_POINTS_DIR}"
echo "  packed:      ${SAMPLE_PACK_DIR}"
echo "  R*-trees:    ${SAMPLE_RS_DIR}"
echo "  KD-trees:    ${SAMPLE_KD_DIR}"
echo "  distances:   ${SAMPLE_PD_PATH}"
