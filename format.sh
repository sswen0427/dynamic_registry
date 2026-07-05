#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "${ROOT_DIR}"

CPP_FILES=()
while IFS= read -r -d '' file; do
  CPP_FILES+=("${file}")
done < <(
  find csrc extension -type f \( -name '*.c' -o -name '*.cc' -o \
    -name '*.cpp' -o -name '*.cxx' -o -name '*.h' -o -name '*.hh' -o \
    -name '*.hpp' -o -name '*.hxx' \) -print0
)

PYTHON_FILES=()
while IFS= read -r -d '' file; do
  PYTHON_FILES+=("${file}")
done < <(
  find . -path './build' -prune -o -path './.git' -prune -o -type f \
    -name '*.py' -print0
)

CMAKE_FILES=()
while IFS= read -r -d '' file; do
  CMAKE_FILES+=("${file}")
done < <(
  find . -path './build' -prune -o -path './.git' -prune -o -type f \
    \( -name 'CMakeLists.txt' -o -name '*.cmake' \) -print0
)

if command -v clang-format >/dev/null 2>&1; then
  if ((${#CPP_FILES[@]} > 0)); then
    echo "formatting C++ files with clang-format"
    clang-format -i "${CPP_FILES[@]}"
  fi
else
  echo "skip C++ formatting: clang-format not found" >&2
fi

if python3 -m black --version >/dev/null 2>&1; then
  if ((${#PYTHON_FILES[@]} > 0)); then
    echo "formatting Python files with black"
    python3 -m black "${PYTHON_FILES[@]}"
  fi
else
  echo "skip Python formatting: python3 -m black not found" >&2
fi

if command -v cmake-format >/dev/null 2>&1; then
  if ((${#CMAKE_FILES[@]} > 0)); then
    echo "formatting CMake files with cmake-format"
    cmake-format -i "${CMAKE_FILES[@]}"
  fi
else
  echo "skip CMake formatting: cmake-format not found" >&2
fi
