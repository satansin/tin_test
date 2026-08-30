#!/usr/bin/env bash
set -euo pipefail

# One-off rename of previously generated ptsample folders:
#   <prefix>_ptsample_512  -> <prefix>_ptsample512_pack
#   <prefix>_ptsample_1024 -> <prefix>_ptsample1024_pack
#   <prefix>_ptsample_2048 -> <prefix>_ptsample2048_pack
#
# Usage:
#   ./scripts/tmp_rename_ptsample.sh          # rename under ../tin_exp
#   ./scripts/tmp_rename_ptsample.sh /path    # rename under a custom root
#   ./scripts/tmp_rename_ptsample.sh --dry-run

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXP_ROOT="${ROOT}/../tin_exp"
DRY_RUN=0

for arg in "$@"; do
  if [[ "${arg}" == "--dry-run" ]]; then
    DRY_RUN=1
  else
    EXP_ROOT="${arg}"
  fi
done

POINT_COUNTS=(512 1024 2048)

echo ">>> tmp_rename_ptsample.sh"
echo "    root: ${EXP_ROOT}"
if [[ "${DRY_RUN}" -eq 1 ]]; then
  echo "    mode: dry-run"
fi
echo ""

if [[ ! -d "${EXP_ROOT}" ]]; then
  echo "Error: directory not found: ${EXP_ROOT}" >&2
  exit 1
fi

renamed=0
skipped=0

while IFS= read -r -d '' old_dir; do
  parent="$(dirname "${old_dir}")"
  base="$(basename "${old_dir}")"
  matched=0
  for n in "${POINT_COUNTS[@]}"; do
    if [[ "${base}" == *_ptsample_"${n}" ]]; then
      prefix="${base%_ptsample_${n}}"
      new_base="${prefix}_ptsample${n}_pack"
      new_dir="${parent}/${new_base}"
      matched=1

      if [[ -e "${new_dir}" ]]; then
        echo "SKIP  ${old_dir}"
        echo "      target exists: ${new_dir}"
        skipped=$((skipped + 1))
        break
      fi

      if [[ "${DRY_RUN}" -eq 1 ]]; then
        echo "DRY   ${old_dir}"
        echo "   -> ${new_dir}"
      else
        mv "${old_dir}" "${new_dir}"
        echo "OK    ${old_dir}"
        echo "   -> ${new_dir}"
      fi
      renamed=$((renamed + 1))
      break
    fi
  done
  if [[ "${matched}" -eq 0 ]]; then
    continue
  fi
done < <(find "${EXP_ROOT}" -type d \( \
  -name '*_ptsample_512' -o \
  -name '*_ptsample_1024' -o \
  -name '*_ptsample_2048' \
\) -print0)

echo ""
echo "Done: renamed=${renamed} skipped=${skipped}"
