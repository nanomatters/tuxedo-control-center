#!/bin/bash
# Test script for DisplayRefreshRateWorker

echo "=== DisplayRefreshRateWorker Test Suite ==="
echo ""

# 1. Check if X11 is detected
echo "1. Testing X11 detection:"
dbus-send --system --print-reply --dest=com.uniwill.uccd \
  /com/uniwill/uccd com.uniwill.uccd.GetIsX11
echo ""

# 2. Get display modes JSON
echo "2. Getting display modes (parsed):"
MODES=$(dbus-send --system --print-reply --dest=com.uniwill.uccd \
  /com/uniwill/uccd com.uniwill.uccd.GetDisplayModesJSON | \
  grep -oP '(?<=string ").*(?="$)')
echo "$MODES" | python3 -m json.tool 2>/dev/null || echo "$MODES"
echo ""

# 3. Check current session type
echo "3. Current session info:"
echo "   XDG_SESSION_TYPE: $XDG_SESSION_TYPE"
echo "   DISPLAY: $DISPLAY"
echo "   XAUTHORITY: $XAUTHORITY"
echo ""

# 4. Run xrandr to compare
echo "4. Current xrandr output (for comparison):"
xrandr --current 2>/dev/null | grep -E "connected|^\s+[0-9]{3,4}x" | head -10
echo ""

# 5. Check if worker is running
echo "5. Checking active profile refresh rate setting:"
PROFILE=$(dbus-send --system --print-reply --dest=com.uniwill.uccd \
  /com/uniwill/uccd com.uniwill.uccd.GetActiveProfileJSON | \
  grep -oP '(?<=string ").*(?="$)')
echo "$PROFILE" | python3 -c "import sys, json; p=json.load(sys.stdin); print(f\"  useRefRate: {p.get('display', {}).get('useRefRate', False)}\n  refreshRate: {p.get('display', {}).get('refreshRate', 'N/A')}\")" 2>/dev/null || echo "  (Unable to parse)"
echo ""

echo "=== Test Complete ==="
echo ""
echo "To manually test refresh rate changes:"
echo "  1. Edit a profile to enable refresh rate control"
echo "  2. Set desired refresh rate in the profile"
echo "  3. Switch to that profile"
echo "  4. Check syslog for: 'Set display mode to...'"
echo ""
echo "Example syslog command:"
echo "  sudo journalctl -f | grep -E 'display|refresh|xrandr'"
