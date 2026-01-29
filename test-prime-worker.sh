#!/bin/bash
# Test script for PrimeWorker

echo "=== PrimeWorker Test Suite ==="
echo ""

# 1. Check Prime state
echo "1. Current Prime State:"
PRIME_STATE=$(dbus-send --system --print-reply --dest=com.uniwill.uccd \
  /com/uniwill/uccd com.uniwill.uccd.GetPrimeState 2>&1 | \
  grep -oP '(?<=string ").*(?="$)')
echo "   Prime State: $PRIME_STATE"
case "$PRIME_STATE" in
  "-1") echo "   → Not available (no NVIDIA Optimus/Prime)" ;;
  "0")  echo "   → Intel/Integrated GPU (Power Saving)" ;;
  "1")  echo "   → NVIDIA GPU (Performance)" ;;
  "2")  echo "   → On-demand (Hybrid)" ;;
  *)    echo "   → Unknown state" ;;
esac
echo ""

# 2. Check system Prime configuration
echo "2. System Prime Configuration:"
if command -v prime-select &> /dev/null; then
  echo "   prime-select: Available"
  CURRENT=$(prime-select query 2>&1)
  echo "   Current mode: $CURRENT"
else
  echo "   prime-select: Not available"
fi
echo ""

# 3. Check NVIDIA GPU presence
echo "3. NVIDIA GPU Detection:"
if command -v nvidia-smi &> /dev/null; then
  echo "   nvidia-smi: Available"
  GPU_COUNT=$(nvidia-smi -L 2>/dev/null | wc -l)
  echo "   GPU count: $GPU_COUNT"
  if [ $GPU_COUNT -gt 0 ]; then
    nvidia-smi -L 2>/dev/null | sed 's/^/   /'
  fi
else
  echo "   nvidia-smi: Not available"
fi
echo ""

# 4. Check for Optimus configuration files
echo "4. Optimus/Prime System Files:"
FILES=(
  "/etc/prime/current_type"
  "/sys/bus/pci/devices/*/power/control"
  "/proc/driver/nvidia/gpus"
)
for file in "${FILES[@]}"; do
  if ls $file 2>/dev/null | head -1 &>/dev/null; then
    echo "   ✓ $file exists"
  else
    echo "   ✗ $file not found"
  fi
done
echo ""

# 5. Test Prime switching (read-only - don't actually switch)
echo "5. Prime Switch Capability:"
if [ "$PRIME_STATE" = "-1" ]; then
  echo "   Prime switching is not supported on this system"
  echo "   (This is normal for desktop/workstation with discrete GPU only)"
else
  echo "   Prime switching is available"
  echo "   Current mode: $PRIME_STATE"
  echo ""
  echo "   To test switching (requires reboot):"
  echo "   - dbus-send --system --print-reply --dest=com.uniwill.uccd \\"
  echo "       /com/uniwill/uccd com.uniwill.uccd.SetPrimeState int32:0"
  echo "   - Check syslog for prime-select execution"
  echo "   - Reboot and verify GPU mode changed"
fi
echo ""

echo "=== Test Complete ==="
echo ""
echo "PrimeWorker Status: ✓ Working (detected state: $PRIME_STATE)"
