#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
CMAKE_BIN="${CMAKE_BIN:-cmake}"

if ! command -v "${CMAKE_BIN}" >/dev/null 2>&1 && [ -x /opt/homebrew/bin/cmake ]; then
  CMAKE_BIN=/opt/homebrew/bin/cmake
fi

"${CMAKE_BIN}" -S "${ROOT_DIR}" -B "${BUILD_DIR}"
"${CMAKE_BIN}" --build "${BUILD_DIR}"

echo "built library: ${BUILD_DIR}/libdynamic_ops_registry.so"
echo "built extension: ${BUILD_DIR}/libcustom_ops.so"
