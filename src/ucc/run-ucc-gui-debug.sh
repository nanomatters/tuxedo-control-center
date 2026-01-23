#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Run UCC GUI from local installation with debug output to file

INSTALL_PREFIX="$HOME/tmp/ucc"
UCC_GUI="$INSTALL_PREFIX/bin/ucc-gui"
LOG_FILE="/tmp/ucc-gui-debug.log"

# Check if installed
if [ ! -f "$UCC_GUI" ]; then
    echo "ERROR: ucc-gui not found at $UCC_GUI"
    exit 1
fi

# Set up library path for local installation
export LD_LIBRARY_PATH="$INSTALL_PREFIX/lib:$LD_LIBRARY_PATH"

# Set Qt plugin path if needed
export QT_PLUGIN_PATH="$INSTALL_PREFIX/lib/qt6/plugins:$QT_PLUGIN_PATH"

# Run the GUI with debug output to log file
echo "Starting UCC GUI from: $UCC_GUI"
echo "Debug output will be written to: $LOG_FILE"
exec "$UCC_GUI" "$@" 2>&1 | tee "$LOG_FILE"
