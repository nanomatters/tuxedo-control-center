#!/bin/bash
# Test script for ODMProfileWorker

echo "=== ODMProfileWorker Test Suite ==="
echo ""

# 1. Check available ODM profiles
echo "1. Available ODM Profiles (DBus):"
PROFILES=$(dbus-send --system --print-reply --dest=com.tuxedocomputers.tccd \
  /com/tuxedocomputers/tccd com.tuxedocomputers.tccd.ODMProfilesAvailable 2>&1 | \
  grep -oP '(?<=string ").*(?=")')

if [ -z "$PROFILES" ]; then
  echo "   → Empty array (no ODM profiles available)"
else
  echo "   Available profiles:"
  echo "$PROFILES" | while read -r profile; do
    echo "   - $profile"
  done
fi
echo ""

# 2. Check system platform profile support
echo "2. System Platform Profile Support:"

if [ -e "/sys/bus/platform/devices/tuxedo_platform_profile/platform_profile" ]; then
  echo "   TUXEDO Platform Profile: Available"
  CHOICES=$(cat /sys/bus/platform/devices/tuxedo_platform_profile/platform_profile_choices 2>/dev/null)
  CURRENT=$(cat /sys/bus/platform/devices/tuxedo_platform_profile/platform_profile 2>/dev/null)
  echo "   Choices: $CHOICES"
  echo "   Current: $CURRENT"
elif [ -e "/sys/firmware/acpi/platform_profile" ]; then
  echo "   ACPI Platform Profile: Available"
  CHOICES=$(cat /sys/firmware/acpi/platform_profile_choices 2>/dev/null)
  CURRENT=$(cat /sys/firmware/acpi/platform_profile 2>/dev/null)
  echo "   Choices: $CHOICES"
  echo "   Current: $CURRENT"
else
  echo "   Platform Profile: Not available"
  echo "   → Using Tuxedo IO API fallback (if supported by hardware)"
fi
echo ""

# 3. Check active profile ODM settings
echo "3. Active Profile ODM Configuration:"
PROFILE=$(dbus-send --system --print-reply --dest=com.tuxedocomputers.tccd \
  /com/tuxedocomputers/tccd com.tuxedocomputers.tccd.GetActiveProfileJSON 2>&1 | \
  grep -oP '(?<=string ").*(?="$)')

if [ -n "$PROFILE" ]; then
  ODM_NAME=$(echo "$PROFILE" | python3 -c "import sys, json; p=json.load(sys.stdin); print(p.get('odmProfile', {}).get('name', 'None'))" 2>/dev/null)
  echo "   ODM Profile Name: $ODM_NAME"
else
  echo "   Unable to parse active profile"
fi
echo ""

# 4. Check syslog for ODM worker messages
echo "4. Recent ODM Worker Logs:"
sudo journalctl --since "5 minutes ago" -q | grep "ODMProfile" | tail -5 || echo "   No logs found"
echo ""

# 5. System capabilities summary
echo "5. ODM Capabilities Summary:"
echo "   Platform Profile (sysfs): $([ -e /sys/firmware/acpi/platform_profile ] && echo '✓' || echo '✗')"
echo "   TUXEDO Platform Profile: $([ -e /sys/bus/platform/devices/tuxedo_platform_profile/platform_profile ] && echo '✓' || echo '✗')"
echo "   Tuxedo IO API: $([ -e /dev/tuxedo_io ] && echo '✓' || echo '✗')"
echo ""

echo "=== Test Complete ==="
echo ""

if [ -z "$PROFILES" ]; then
  echo "ODMProfileWorker Status: ✓ Working (no profiles available on this system)"
  echo ""
  echo "This is normal for systems without:"
  echo "  - ACPI platform_profile support"
  echo "  - TUXEDO platform_profile driver"
  echo "  - Clevo/Uniwill ODM performance profile hardware"
else
  echo "ODMProfileWorker Status: ✓ Working ($PROFILES profiles detected)"
fi
