# Testing the UCC System Monitor Widget

## Quick Testing Methods

### 1. **Test with Direct QML Execution** (if Qt Quick tools available)
```bash
cd /opt/devel/git/tuxedo/tuxedo-control-center/ucc/ucc-widgets/system-monitor/package/contents/ui
qml6 main.qml
# or
qml main.qml
```

### 2. **Install and Test as Plasma Widget**
```bash
cd /opt/devel/git/tuxedo/tuxedo-control-center/ucc/build
sudo cmake --install .

# Then:
# - Log out and back in, or restart Plasma
# - Add "UCC System Monitor" widget to your desktop/panel
# - Right-click panel → Add widgets → Search "System Monitor"
```

### 3. **Verify DBus Integration**
Test if tccd-ng is responding (mock data is used if service unavailable):

```bash
# Test CPU Temperature method (will fail with "Unknown method" - expected, method not yet implemented in tccd-ng)
dbus-send --system --print-reply --dest=com.tuxedocomputers.tccd \
    /com/tuxedocomputers/tccd \
    com.tuxedocomputers.tccd.GetCpuTemperature

# Verify tccd-ng is running
dbus-send --system --print-reply --dest=com.tuxedocomputers.tccd \
    /com/tuxedocomputers/tccd \
    org.freedesktop.DBus.Introspectable.Introspect | grep -E "method|interface" | head -20
```

### 4. **Test the GUI Application**
The SystemMonitor class is integrated with the GUI app. Build and run it:

```bash
cd /opt/devel/git/tuxedo/tuxedo-control-center/ucc/build
./ucc-gui/ucc-gui
```

This will test the C++ backend integration with real tccd-ng DBus calls.

## What Was Implemented

### Backend (C++)
- **TccdClient**: Added 7 new DBus methods:
  - `getCpuTemperature()` - int (°C)
  - `getGpuTemperature()` - int (°C)
  - `getCpuFrequency()` - int (MHz)
  - `getGpuFrequency()` - int (MHz)
  - `getCpuPower()` - int (W)
  - `getGpuPower()` - int (W)
  - `getFanSpeedRPM()` - int (RPM)

- **SystemMonitor**: Added Q_PROPERTY bindings for all metrics
  - Updates every 2 seconds
  - Emits signals on property changes
  - Used by GUI application

### Widget (QML)
- **system-monitor widget**: Plasma widget displaying:
  - CPU: Temperature, Frequency, Power
  - GPU: Temperature, Frequency, Power
  - Fan Speed (RPM)
  - CPU Usage (%)
  
- Features:
  - DBus integration with QtDBus
  - Fallback to mock data if service unavailable
  - Updates every 2 seconds
  - Simple grid layout

## Current Status

✅ **Build**: Successful
✅ **DBus Interface**: Defined
✅ **QML Widget**: Complete with display and DBus integration
✅ **C++ Backend**: Ready to call tccd-ng methods
⏳ **tccd-ng Methods**: Need to be implemented in tccd-ng daemon to fetch real metrics

## Next Steps

1. Implement the methods in tccd-ng daemon:
   - `GetCpuTemperature()` → Read from /sys/class/hwmon/
   - `GetGpuTemperature()` → Read from GPU drivers
   - `GetCpuFrequency()` → Read from /sys/devices/system/cpu/
   - `GetGpuFrequency()` → Read from GPU drivers
   - `GetCpuPower()` → Calculate from power monitoring
   - `GetGpuPower()` → Read from power ICs
   - `GetFanSpeed()` → Read from hwmon interface

2. Once implemented in tccd-ng, widget will show real metrics

## Files Modified

- `/opt/devel/git/tuxedo/tuxedo-control-center/ucc/libucc-dbus/TccdClient.hpp`
- `/opt/devel/git/tuxedo/tuxedo-control-center/ucc/libucc-dbus/TccdClient.cpp`
- `/opt/devel/git/tuxedo/tuxedo-control-center/ucc/ucc-gui/SystemMonitor.hpp`
- `/opt/devel/git/tuxedo/tuxedo-control-center/ucc/ucc-gui/SystemMonitor.cpp`
- `/opt/devel/git/tuxedo/tuxedo-control-center/ucc/ucc-widgets/system-monitor/package/contents/ui/main.qml`
- `/opt/devel/git/tuxedo/tuxedo-control-center/ucc/ucc-widgets/system-monitor/package/contents/ui/SystemMetricsProvider.qml` (created)
