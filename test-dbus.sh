#!/bin/bash
# Test script to check DBus interface

echo "=== Testing TCCD DBus Interface ==="
echo ""

echo "1. Checking if service is registered..."
dbus-send --system --print-reply \
  --dest=org.freedesktop.DBus \
  /org/freedesktop/DBus \
  org.freedesktop.DBus.ListNames | grep -q "com.tuxedocomputers.tccd"

if [ $? -eq 0 ]; then
    echo "✓ Service is registered"
else
    echo "✗ Service is NOT registered"
    exit 1
fi

echo ""
echo "2. Testing GetDeviceName..."
dbus-send --system --print-reply \
  --dest=com.tuxedocomputers.tccd \
  /com/tuxedocomputers/tccd \
  com.tuxedocomputers.tccd.GetDeviceName

echo ""
echo "3. Testing TuxedoWmiAvailable..."
dbus-send --system --print-reply \
  --dest=com.tuxedocomputers.tccd \
  /com/tuxedocomputers/tccd \
  com.tuxedocomputers.tccd.TuxedoWmiAvailable

echo ""
echo "4. Testing GetProfilesJSON..."
dbus-send --system --print-reply \
  --dest=com.tuxedocomputers.tccd \
  /com/tuxedocomputers/tccd \
  com.tuxedocomputers.tccd.GetProfilesJSON

echo ""
echo "5. Testing GetDisplayModesJSON..."
dbus-send --system --print-reply \
  --dest=com.tuxedocomputers.tccd \
  /com/tuxedocomputers/tccd \
  com.tuxedocomputers.tccd.GetDisplayModesJSON

echo ""
echo "6. Introspecting interface..."
dbus-send --system --print-reply \
  --dest=com.tuxedocomputers.tccd \
  /com/tuxedocomputers/tccd \
  org.freedesktop.DBus.Introspectable.Introspect
