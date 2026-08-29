#!/usr/bin/env bash
set -euo pipefail

# Configure and build tin_test with a CMake preset.
#
# Usage:
#   ./build.sh           # release (default)
#   ./build.sh release
#   ./build.sh debug

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRESET="${1:-release}"

if [[ "${PRESET}" != "release" && "${PRESET}" != "debug" ]]; then
  echo "Usage: $0 [release|debug]" >&2
  exit 1
fi

cd "${ROOT}"

echo ">>> configure preset: ${PRESET}"
cmake --preset "${PRESET}"

echo ">>> build preset: ${PRESET}"
cmake --build --preset "${PRESET}" --parallel

BIN="${ROOT}/build/${PRESET}/tin_test"
echo ">>> binary: ${BIN}"
