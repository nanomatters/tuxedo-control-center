# Uniwill Control Center (UCC)

Modern Qt6/KDE C++20 application suite for Uniwill laptop control.

## Components

- **libucc-dbus**: Shared library for communicating with tccd-ng daemon via DBus
- **ucc-gui**: Main GUI application with QML interface
- **ucc-tray**: System tray applet for quick access
- **ucc-widgets**: Plasma desktop widgets (system monitor, profile switcher)

## Features

- Profile management (view, create, switch profiles)
- Real-time system monitoring (CPU, GPU, fan speeds)
- Display brightness control
- Webcam toggle
- Fn Lock control
- Power management
- Hardware control through tccd-ng daemon

## Dependencies

### Build Requirements
- CMake >= 3.20
- GCC with C++20 support
- Qt6 (Core, Gui, Widgets, Qml, Quick, QuickControls2)
- KDE Frameworks 6
- sdbus-c++ >= 2.0

### Runtime Requirements
- tccd-ng daemon (from parent project)
- Qt6 runtime libraries
- KDE Plasma (for widgets)

## Building

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build .
sudo cmake --install .
```

### Build Options

- `BUILD_GUI=ON/OFF` - Build main GUI application (default: ON)
- `BUILD_TRAY=ON/OFF` - Build system tray applet (default: ON)
- `BUILD_WIDGETS=ON/OFF` - Build Plasma widgets (default: ON)

Example:
```bash
cmake .. -DBUILD_WIDGETS=OFF  # Skip widget building
```

## Running

### Main Application
```bash
ucc-gui
```

### System Tray
```bash
ucc-tray
```

The tray applet can be added to autostart via the desktop file:
`~/.config/autostart/ucc-tray.desktop`

### Plasma Widgets
After installation, widgets can be added through Plasma's widget manager:
- Right-click on desktop → Add Widgets
- Search for "UCC System Monitor" or "UCC Profile Switcher"

## Architecture

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   ucc-gui   │────▶│ libucc-dbus │────▶│   tccd-ng   │
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

All components communicate with the tccd-ng daemon through the shared libucc-dbus library.

## Development

### Project Structure
```
ucc/
├── CMakeLists.txt              # Root build configuration
├── README.md                   # This file
├── libucc-dbus/               # Shared DBus client library
│   ├── TccdClient.hpp
│   ├── TccdClient.cpp
│   └── CMakeLists.txt
├── ucc-gui/                   # Main GUI application
│   ├── main.cpp
│   ├── ProfileManager.{hpp,cpp}
│   ├── SystemMonitor.{hpp,cpp}
│   ├── qml/main.qml
│   └── CMakeLists.txt
├── ucc-tray/                  # System tray applet
│   ├── main.cpp
│   └── CMakeLists.txt
└── ucc-widgets/               # Plasma widgets
    ├── system-monitor/
    └── profile-switcher/
```

### Code Style
- C++20 modern features (std::optional, concepts, ranges, etc.)
- RAII and smart pointers
- Qt naming conventions for Qt code
- Standard C++ naming for core logic

## License

GPL-3.0-or-later

## Contributing

This project follows the same guidelines as the parent tccd-ng project.
