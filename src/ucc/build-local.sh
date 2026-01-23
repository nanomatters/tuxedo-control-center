#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Build UCC with local installation to ~/tmp/ucc

set -e

BUILD_DIR="build"
BUILD_TYPE="${1:-RelWithDebInfo}"
INSTALL_PREFIX="$HOME/tmp/ucc"
CMAKELISTS_BACKUP="CMakeLists.txt.bak"

echo "=== Building Uniwill Control Center (Local Install) ==="
echo "Build type: $BUILD_TYPE"
echo "Install prefix: $INSTALL_PREFIX"
echo ""

# Create install directory
mkdir -p "$INSTALL_PREFIX"

# Backup and modify CMakeLists.txt to use our install prefix
if [ ! -f "$CMAKELISTS_BACKUP" ]; then
    cp CMakeLists.txt "$CMAKELISTS_BACKUP"
fi

# Temporarily modify the install prefix in CMakeLists.txt
sed -i "s|set( CMAKE_INSTALL_PREFIX \".*\" CACHE PATH \"Installation prefix\" FORCE )|set( CMAKE_INSTALL_PREFIX \"$INSTALL_PREFIX\" CACHE PATH \"Installation prefix\" FORCE )|" CMakeLists.txt

# Ensure cleanup happens even on error
cleanup() {
    if [ -f "$CMAKELISTS_BACKUP" ]; then
        mv "$CMAKELISTS_BACKUP" CMakeLists.txt
    fi
}
trap cleanup EXIT

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Remove cache to ensure install prefix is properly set
rm -f CMakeCache.txt

# Configure
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build . -j$(nproc)

echo ""
echo "=== Build Complete ==="
echo ""
echo "To install:"
echo "  cd build && cmake --install ."
echo ""
echo "After installation, run GUI with:"
echo "  $INSTALL_PREFIX/bin/ucc-gui"
echo ""
echo "Or use the provided run script:"
echo "  ./run-ucc-gui.sh"
