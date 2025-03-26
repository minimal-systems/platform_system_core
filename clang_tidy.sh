#!/bin/bash
# run-clang-tidy.sh – Batch run clang-tidy on all source files in the init project.

set -e

# Paths
BUILD_DIR="./build"
SRC_DIR="."  # assuming the CMakeLists.txt is the project root
CHECKS="clang-analyzer-*,google-*,modernize-*"

# Step 1: Ensure compile_commands.json exists
if [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
  echo "Generating build with compile_commands.json..."
  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B "${BUILD_DIR}" -S "${SRC_DIR}"
fi

# Step 2: Run clang-tidy on all listed INIT_SOURCES
echo "Running clang-tidy on all .cpp files listed in CMakeLists.txt..."

find "${SRC_DIR}" -maxdepth 1 -name '*.cpp' | \
  xargs -P"$(nproc)" -I{} clang-tidy -p "${BUILD_DIR}" --checks="${CHECKS}" --fix --format-style=file {}

echo "✅ clang-tidy completed on all files."
