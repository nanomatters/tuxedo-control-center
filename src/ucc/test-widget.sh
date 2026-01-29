#!/bin/bash
# Test script for system-monitor widget

WIDGET_PATH="/opt/devel/git/tuxedo/tuxedo-control-center/src/ucc/ucc-widgets/system-monitor/package/contents/ui"

# Try to run with Qt Quick tester (if available)
if command -v qml6 &> /dev/null; then
    echo "Testing with qml6..."
    cd "$WIDGET_PATH"
    qml6 main.qml
elif command -v qml &> /dev/null; then
    echo "Testing with qml..."
    cd "$WIDGET_PATH"
    qml main.qml
else
    echo "QML testers not available, using DBus introspection instead..."
    
    # Test DBus interface introspection
    echo "Available DBus methods in com.uniwill.uccd:"
    dbus-send --system --print-reply --dest=com.tuxedocomputers.tccd \
        /com/tuxedocomputers/tccd \
        org.freedesktop.DBus.Introspectable.Introspect | grep -E "<method|<property" | head -20
fi
