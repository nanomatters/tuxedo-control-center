#!/bin/bash
# Run UCC GUI and capture debug output

INSTALL_PREFIX="$HOME/tmp/ucc"
export LD_LIBRARY_PATH="$INSTALL_PREFIX/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$INSTALL_PREFIX/lib/qt6/plugins:$QT_PLUGIN_PATH"

# Run GUI for 10 seconds and capture output
timeout 10s "$INSTALL_PREFIX/bin/ucc-gui" 2>&1 | tee /tmp/ucc-debug.log

echo ""
echo "=== Debug output saved to /tmp/ucc-debug.log ==="
echo ""
echo "=== Last 50 lines of output: ==="
tail -50 /tmp/ucc-debug.log
