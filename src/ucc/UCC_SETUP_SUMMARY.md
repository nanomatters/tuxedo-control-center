# UCC Setup Complete - Summary

## What is UCC?

**Uniwill Control Center (UCC)** is a modern Qt6/KDE C++20 application suite for Uniwill laptop control, designed as a native replacement for the Electron-based TCC GUI. It communicates with the tccd-ng daemon via DBus.

## Architecture

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   ucc-gui   │────▶│ libucc-dbus │────▶│   tccd-ng   │
│  (Qt/QML)   │     │  (Qt DBus)  │     │   (daemon)  │
└─────────────┘     └─────────────┘     └─────────────┘
       ▲                    ▲
       │                    │
┌─────────────┐     ┌─────────────┐
│  ucc-tray   │     │ ucc-widgets │
│ (Sys Tray)  │     │  (Plasma)   │
└─────────────┘     └─────────────┘
```

## Components

### 1. libucc-dbus (Shared Library)
- Qt-native DBus client library
- Type-safe method calls with C++20 templates
- Qt signals for daemon events
- Auto-reconnection support

### 2. ucc-gui (Main Application)
- QML-based modern UI
- Profile management (view, create, switch)
- Real-time system monitoring (CPU, RAM, fans, temps)
- Display brightness control
- Webcam and Fn Lock toggles

### 3. ucc-tray (System Tray)
- Quick access tray icon
- Profile quick-switch menu
- Webcam/Fn Lock toggles
- Launch main GUI

### 4. ucc-widgets (Plasma Widgets)
- System Monitor widget
- Profile Switcher widget
- Native KDE Plasma 6 integration

## Installation Details

**Install Location:** `~/tmp/src/ucc/`

**Structure:**
```
~/tmp/src/ucc/
├── bin/
│   ├── ucc-gui          (3.7 MB - Main GUI application)
│   └── ucc-tray         (957 KB - System tray applet)
├── lib/
│   ├── libucc-dbus.so.0.1.0  (2.3 MB - DBus library)
│   ├── libucc-dbus.so.0 → libucc-dbus.so.0.1.0
│   └── libucc-dbus.so → libucc-dbus.so.0
├── include/src/ucc/
│   └── TccdClient.hpp
└── share/
    ├── applications/
    │   └── ucc-tray.desktop
    └── plasma/plasmoids/
        ├── org.uniwill.ucc.systemmonitor/
        └── org.uniwill.ucc.profileswitcher/
```

## How to Use from VSCode

### Option 1: VSCode Tasks (Recommended)

Open the Command Palette (`Ctrl+Shift+P`) → **Tasks: Run Task** → Select:

1. **UCC: Build** - Build with local install prefix
2. **UCC: Install** - Install to ~/tmp/ucc
3. **UCC: Run GUI** - Launch the GUI application
4. **UCC: Build, Install & Run** - Complete workflow

### Option 2: Command Line

```bash
# From workspace root
cd ucc

# Build
./build-local.sh

# Install
cd build && cmake --install .

# Run
cd .. && ./run-ucc-gui.sh
```

## Scripts Created

### build-local.sh
- Configures CMake with `CMAKE_INSTALL_PREFIX=$HOME/tmp/ucc`
- Temporarily modifies CMakeLists.txt to override hardcoded prefix
- Builds all components (library, GUI, tray, widgets)
- Automatically restores CMakeLists.txt

### run-ucc-gui.sh
- Sets up proper environment (`LD_LIBRARY_PATH`, `QT_PLUGIN_PATH`)
- Checks if installation exists
- Launches ucc-gui from ~/tmp/src/ucc/bin/

## Technology Stack

- **Language:** C++20
- **GUI Framework:** Qt6 (6.10.1) with QML
- **DBus:** QtDBus (native Qt integration)
- **Widgets:** KDE Plasma 6
- **Build System:** CMake 3.20+

## Key Features

✓ Native performance (no Electron overhead)  
✓ Seamless KDE Plasma integration  
✓ Type-safe DBus communication  
✓ Real-time system monitoring  
✓ Profile management  
✓ Hardware control (webcam, fn-lock, brightness)  
✓ System tray quick access  
✓ Desktop widgets  

## Requirements

**Build:**
- CMake >= 3.20
- GCC with C++20 support
- Qt6 (Core, Gui, Widgets, DBus, Qml, Quick, QuickControls2)
- KDE Frameworks 6

**Runtime:**
- tccd-ng daemon (must be running)
- Qt6 runtime libraries

## Notes

- tccd-ng daemon must be running for UCC to function
- All DBus communication goes through libucc-dbus
- The build script automatically handles CMakeLists.txt modification
- Installation is fully local - no system directories modified
- Run script handles library path configuration automatically

## References

- Main README: [src/ucc/README.md](src/ucc/README.md)
- Project Status: [src/ucc/PROJECT_STATUS.md](src/ucc/PROJECT_STATUS.md)
- Local Install Guide: [src/ucc/INSTALL_LOCAL.md](src/ucc/INSTALL_LOCAL.md)
