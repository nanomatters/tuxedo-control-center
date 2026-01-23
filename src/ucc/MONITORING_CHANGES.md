# SystemMonitor Performance Polling Changes

## Summary

Modified the UCC SystemMonitor to poll CPU and GPU data every **500ms** (instead of 2 seconds) and **only when the dashboard tab is selected**.

## Changes Made

### 1. SystemMonitor.hpp
- Added `monitoringActive` property (Q_PROPERTY)
- Added `setMonitoringActive()` public slot
- Added `monitoringActiveChanged()` signal
- Added `m_monitoringActive` member variable

### 2. SystemMonitor.cpp
- Changed polling interval from **2000ms to 500ms**
- Modified constructor to **not** start timer automatically
- Timer now starts/stops based on `monitoringActive` state
- Added `setMonitoringActive()` implementation:
  - When `true`: Performs immediate update and starts 500ms timer
  - When `false`: Stops the timer

### 3. main.qml
- Added `onCurrentIndexChanged` handler to StackLayout
- Automatically sets `systemMonitor.monitoringActive = true` when dashboard tab (index 0) is selected
- Automatically sets `systemMonitor.monitoringActive = false` when other tabs are selected

## Behavior

### Dashboard Tab (Index 0) - Active
- ✅ Polls CPU temperature, frequency, power every 500ms
- ✅ Polls GPU temperature, frequency, power every 500ms
- ✅ Polls fan speeds (CPU and GPU) every 500ms
- ✅ Updates display brightness, webcam, and fn-lock status

### Other Tabs (Index 1-3) - Inactive
- ❌ No polling occurs
- ❌ Timer is stopped
- ❌ Reduces CPU usage when not viewing dashboard

## Data Being Monitored

When active, the following metrics are polled every 500ms:

1. **CPU Metrics**
   - Temperature (°C)
   - Frequency (MHz)
   - Power consumption (W)

2. **GPU Metrics**
   - Temperature (°C)
   - Frequency (MHz)
   - Power consumption (W)

3. **Cooling**
   - CPU fan speed (RPM)
   - GPU fan speed (RPM)

4. **Controls**
   - Display brightness (0-100%)
   - Webcam status (enabled/disabled)
   - Fn Lock status (enabled/disabled)

## Performance Impact

- **Before**: Polling every 2 seconds regardless of tab
- **After**: Polling every 500ms ONLY when dashboard visible

This provides:
- ✅ 4x faster updates when viewing dashboard (more responsive)
- ✅ Zero polling overhead when not viewing dashboard (better efficiency)
- ✅ Smooth real-time monitoring experience

## Testing

Build and run:
```bash
cd /opt/devel/git/tuxedo/tuxedo-control-center/ucc
./build-local.sh
cd build && cmake --install . && cd ..
./run-ucc-gui.sh
```

Or use VSCode task: **UCC: Build, Install & Run**

## Technical Details

The monitoring is controlled through Qt's property binding system:
- QML automatically updates `monitoringActive` property when tab changes
- C++ SystemMonitor receives the property change via `setMonitoringActive()`
- Timer starts/stops based on the new state
- All updates are emitted via Qt signals for reactive UI updates
