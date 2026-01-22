#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Quick build script for UCC

set -e

BUILD_DIR="build"
BUILD_TYPE="${1:-RelWithDebInfo}"

echo "=== Building Uniwill Control Center ==="
echo "Build type: $BUILD_TYPE"
echo ""

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build . -j$(nproc)

echo ""
echo "=== Build Complete ==="
echo "To install: sudo cmake --install ."
echo "To run GUI: ./ucc-gui/ucc-gui"
echo "To run tray: ./ucc-tray/ucc-tray"
