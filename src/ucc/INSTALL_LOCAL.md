# UCC Local Installation Guide

This guide explains the local installation setup for UCC (Uniwill Control Center) that installs to `~/tmp/ucc` instead of system directories.

## Quick Start from VSCode

Use the following VSCode tasks (Terminal → Run Task):

1. **UCC: Build** - Builds UCC with local install prefix
2. **UCC: Install** - Installs to ~/tmp/ucc
3. **UCC: Run GUI** - Launches the GUI application
4. **UCC: Build, Install & Run** - Complete workflow in one command

## Manual Commands

### Build
```bash
cd ucc
./build-local.sh
```

### Install
```bash
cd src/ucc/build
cmake --install .
```

### Run GUI
```bash
cd ucc
./run-ucc-gui.sh
```

## Project Structure

```
src/ucc/
├── build-local.sh          # Build script with ~/tmp/ucc install prefix
├── run-ucc-gui.sh          # Run script with proper environment setup
├── libucc-dbus/            # DBus client library for uccd
│   ├── TccdClient.hpp
│   └── TccdClient.cpp
├── ucc-gui/                # Main Qt/QML GUI application
│   ├── main.cpp
│   ├── MainWindow.{hpp,cpp}
│   ├── ProfileManager.{hpp,cpp}
│   ├── SystemMonitor.{hpp,cpp}
│   └── qml/
├── ucc-tray/               # System tray applet
│   └── main.cpp
└── ucc-widgets/            # KDE Plasma widgets
    ├── system-monitor/
    └── profile-switcher/
```

## Architecture

UCC is a Qt6/KDE-based application suite that communicates with the tccd-ng daemon via DBus:

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   ucc-gui   │────▶│ libucc-dbus │────▶│   uccd   │
└─────────────┘     └─────────────┘     └─────────────┘
                            ▲
                            │
┌─────────────┐             │
│  ucc-tray   │─────────────┤
└─────────────┘             │
                            │
┌─────────────┐             │
│ ucc-widgets │─────────────┘
└─────────────┘
```

## Components

### libucc-dbus
Shared Qt-native DBus client library for communicating with uccd daemon.

**Features:**
- Type-safe method calls with templates
- Qt signals for daemon events
- Auto-reconnection support
- Optional return types for error handling

### ucc-gui
Main control center application with QML-based UI.

**Features:**
- Profile management (view, create, switch)
- System monitoring (CPU, RAM, fan, temps)
- Display brightness control
- Webcam toggle
- Fn Lock control

### ucc-tray
System tray applet for quick access.

**Features:**
- Profile quick-switch menu
- Webcam/Fn Lock toggles
- Launch main GUI
- Status notifications

### ucc-widgets
KDE Plasma desktop widgets.

**Components:**
- System Monitor widget (CPU, RAM, fan, temps)
- Profile Switcher widget

## Dependencies

**Build Requirements:**
- CMake >= 3.20
- GCC with C++20 support
- Qt6 (Core, Gui, Widgets, DBus, Qml, Quick, QuickControls2)
- KDE Frameworks 6 (for widgets)

**Runtime Requirements:**
- uccd daemon (must be running)
- Qt6 runtime libraries

## Installation Paths

When using the local installation:

- **Binaries:** `~/tmp/src/ucc/bin/`
  - `ucc-gui`
  - `ucc-tray`
  
- **Libraries:** `~/tmp/src/ucc/lib/`
  - `libucc-dbus.so`

- **Widgets:** `~/tmp/src/ucc/share/plasma/plasmoids/`

## Notes

- The tccd-ng daemon must be running for UCC to function
- The run script automatically sets `LD_LIBRARY_PATH` for local libraries
- Build type defaults to `RelWithDebInfo` (optimized with debug symbols)
