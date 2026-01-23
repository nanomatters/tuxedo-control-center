# Quick Test Guide for UCC System Monitor Widget

## Run the Test Suite
```bash
cd /opt/devel/git/tuxedo/tuxedo-control-center
./test-widget.sh
```

## Option 1: Test GUI Application
The GUI application is the primary way to test the SystemMonitor integration:

```bash
cd /opt/devel/git/tuxedo/tuxedo-control-center/ucc/build
./ucc-gui/ucc-gui
```

**What it tests:**
- C++ SystemMonitor class integration
- DBus calls to tccd-ng (will fail gracefully with "Unknown method" until tccd-ng implements them)
- GUI rendering with Plasma components
- Signal/slot connections

**Expected output:**
```
DBus call failed: "GetCpuTemperature" - "Unknown method GetCpuTemperature..."
DBus call failed: "GetCpuFrequency" - "Unknown method GetCpuFrequency..."
...
```

This is expected and normal - once tccd-ng implements these methods, real values will appear.

## Option 2: Test Plasma Widget
To test the Plasma widget directly:

### Install the widget:
```bash
cd /opt/devel/git/tuxedo/tuxedo-control-center/ucc/build
sudo cmake --install .
```

### Add to Plasma desktop:
1. Right-click on desktop or panel
2. Select "Add Widgets..."
3. Search for "System Monitor" or "UCC"
4. Click "UCC System Monitor"
5. Widget will appear showing system metrics (mock data if tccd-ng methods not available)

### Or via KDE Plasma Activities:
- Open Plasma widget browser
- Add "UCC System Monitor"
- Widget updates every 2 seconds

## Option 3: Test DBus Interface Directly
```bash
# Check if tccd-ng service is running
dbus-send --system --print-reply --dest=com.tuxedocomputers.tccd \
    /com/tuxedocomputers/tccd \
    org.freedesktop.DBus.Introspectable.Introspect

# Try calling a method (will fail until implemented in tccd-ng)
dbus-send --system --print-reply --dest=com.tuxedocomputers.tccd \
    /com/tuxedocomputers/tccd \
    com.tuxedocomputers.tccd.GetCpuTemperature
```

## Files Being Tested

| Component | File | Status |
|-----------|------|--------|
| Backend Library | `ucc/libucc-dbus/TccdClient.cpp` | ✅ Implemented |
| GUI Integration | `ucc/ucc-gui/SystemMonitor.cpp` | ✅ Implemented |
| Plasma Widget | `ucc/ucc-widgets/system-monitor/package/contents/ui/main.qml` | ✅ Implemented |
| tccd-ng Methods | `tccd-ng` daemon | ⏳ Pending implementation |

## Expected Behavior

### With tccd-ng Methods Implemented:
- GUI shows live CPU/GPU temperatures, frequencies, power
- Widget displays real system metrics
- Updates every 2 seconds

### Currently (Methods Not Yet Implemented):
- GUI shows graceful error messages in console
- Widget shows mock data (random values)
- No crashes, clean fallback behavior

## Troubleshooting

**"Unknown method" errors:**
- This is expected - tccd-ng doesn't implement these methods yet
- Widget will show mock data instead
- No action needed

**GUI won't start:**
- Check Qt6 dependencies: `qt6-base`
- Check Plasma: `kde-plasma-framework`
- Check DBus: `libdbus-1-3`

**Widget not appearing:**
- Clear Plasma cache: `rm -rf ~/.cache/plasmashell/`
- Restart Plasma: `killall plasmashell; kquitapp6 plasmashell; plasmashell &`
- Reinstall: `cd build && sudo cmake --install .`

## Next Steps

1. **Implement DBus methods in tccd-ng:**
   - `GetCpuTemperature()` - Read from `/sys/class/hwmon/`
   - `GetGpuTemperature()` - Read from GPU drivers
   - `GetCpuFrequency()` - Read from `/sys/devices/system/cpu/cpu*/cpufreq/`
   - `GetGpuFrequency()` - Read from GPU drivers
   - `GetCpuPower()` - Calculate from hwmon sensors
   - `GetGpuPower()` - Read from power monitoring ICs
   - `GetFanSpeed()` - Read from `/sys/class/hwmon/*/fan*_input`

2. **Once implemented:**
   - Widget will automatically use real data
   - No changes needed to UCC code
   - Restart GUI or widget for updates

## Success Criteria

- [ ] All 7 methods callable from GUI without crashes
- [ ] DBus calls succeed (return temperature/frequency/power values)
- [ ] Widget displays metrics in 2-second updates
- [ ] Values match system reality
- [ ] Graceful handling if tccd-ng unavailable
