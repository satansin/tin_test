#!/usr/bin/env bash
set -euo pipefail

# Compute pairwise distance matrices (vertex algorithm) for every dataset under
# ../tin_exp/datasets_normalized/, using prebuilt R*-trees from datasets_norm_rs/.
#
# Usage:
#   ./scripts/compute_pd_datasets.sh [small|full]
#
# Assumptions:
#   - ../tin_exp/datasets_normalized/<name>/ contains normalized .ply files
#   - ../tin_exp/datasets_norm_rs/<name>/combined.rstree exists (run build_rs_datasets.sh)
#
# Output (per dataset <name>):
#   - ../tin_exp/datasets_norm_pd/<name>/pairwise_distances_vertex.txt

MODE="${1:-small}"
shopt -s nullglob

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIN_TEST="${TIN_TEST_BIN:-}"
if [[ -z "${TIN_TEST}" ]]; then
  if [[ -x "${ROOT}/build/release/tin_test" ]]; then
    TIN_TEST="${ROOT}/build/release/tin_test"
  else
    TIN_TEST="${ROOT}/build/debug/tin_test"
  fi
fi

NORM_ROOT="${ROOT}/../tin_exp/datasets_normalized"
RS_ROOT="${ROOT}/../tin_exp/datasets_norm_rs"
PD_ROOT="${ROOT}/../tin_exp/datasets_norm_pd"

if [[ ! -d "${NORM_ROOT}" ]]; then
  echo "error: normalized datasets not found: ${NORM_ROOT}" >&2
  echo "Run ./scripts/normalize_datasets.sh first." >&2
  exit 1
fi

if [[ ! -d "${RS_ROOT}" ]]; then
  echo "error: R*-tree datasets not found: ${RS_ROOT}" >&2
  echo "Run ./scripts/build_rs_datasets.sh first." >&2
  exit 1
fi

mkdir -p "${PD_ROOT}"

compute_pd_dir() {
  local in_dir="$1"
  local rs_dir="$2"
  local out_path="$3"
  local max_objects="$4"
  mkdir -p "$(dirname "${out_path}")"
  if [[ "${max_objects}" -gt 0 ]]; then
    "${TIN_TEST}" pd --input-dir "${in_dir}" --rs-dir "${rs_dir}" \
      --algorithm vertex --output "${out_path}" --max-objects "${max_objects}"
  else
    "${TIN_TEST}" pd --input-dir "${in_dir}" --rs-dir "${rs_dir}" \
      --algorithm vertex --output "${out_path}"
  fi
}

process_dataset() {
  local name="$1"
  local max_objects="$2"
  local in_dir="${NORM_ROOT}/${name}"
  local rs_dir="${RS_ROOT}/${name}"
  local out_path="${PD_ROOT}/${name}/pairwise_distances_vertex.txt"

  if [[ ! -d "${in_dir}" ]]; then
    return 0
  fi
  if [[ ! -f "${rs_dir}/combined.rstree" ]]; then
    echo "warning: skipping ${name}: missing ${rs_dir}/combined.rstree" >&2
    return 0
  fi

  echo "--- ${name} ---"
  compute_pd_dir "${in_dir}" "${rs_dir}" "${out_path}" "${max_objects}"
}

echo "=== Pairwise distances (vertex) from datasets_normalized ==="
if [[ "${MODE}" == "full" ]]; then
  for d in "${NORM_ROOT}"/*; do
    [[ -d "${d}" ]] || continue
    process_dataset "$(basename "${d}")" 0
  done
else
  for name in synthetic_objects100_vertices200 synthetic_objects100_vertices500 \
    synthetic_objects1000_vertices200; do
    process_dataset "${name}" 0
  done
  for d in "${NORM_ROOT}"/*; do
    [[ -d "${d}" ]] || continue
    name="$(basename "${d}")"
    [[ "${name}" == synthetic_* ]] && continue
    process_dataset "${name}" 100
  done
fi

echo "Done: ${PD_ROOT}"
