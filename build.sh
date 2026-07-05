#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}"

echo "built library: ${BUILD_DIR}/libdynamic_ops_registry.so"
echo "built extension: ${BUILD_DIR}/libcustom_ops.so"
