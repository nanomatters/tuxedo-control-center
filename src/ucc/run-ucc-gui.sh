#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Run UCC GUI from local installation

INSTALL_PREFIX="$HOME/tmp/ucc"
UCC_GUI="$INSTALL_PREFIX/bin/ucc-gui"

# Check if installed
if [ ! -f "$UCC_GUI" ]; then
    echo "ERROR: ucc-gui not found at $UCC_GUI"
    echo ""
    echo "Please build and install first:"
    echo "  cd $(dirname "$0")"
    echo "  ./build-local.sh"
    echo "  cd build"
    echo "  cmake --install ."
    exit 1
fi

# Set up library path for local installation
export LD_LIBRARY_PATH="$INSTALL_PREFIX/lib:$LD_LIBRARY_PATH"

# Set Qt plugin path if needed
export QT_PLUGIN_PATH="$INSTALL_PREFIX/lib/qt6/plugins:$QT_PLUGIN_PATH"

# Run the GUI
echo "Starting UCC GUI from: $UCC_GUI"
exec "$UCC_GUI" "$@"
