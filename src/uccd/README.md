# uccd - TUXEDO Control Center Daemon (Next Generation)

A modern, pure C++20 implementation of the TUXEDO Control Center daemon (`tccd`).

## Overview

`uccd` is the next-generation system daemon for TUXEDO Control Center, written entirely in modern C++20 to replace the TypeScript-based `tccd`. It provides:

- Hardware monitoring and control
- Fan speed management
- Power profile management
- Display brightness control
- Webcam control
- TDP (Thermal Design Power) management
- DBus interface for system integration

## Features

### C++20 Improvements
- Modern language features (`std::string_view`, ranges, concepts, etc.)
- Improved performance with native compilation
- Better memory safety with smart pointers
- Structured concurrency and error handling

### Architecture
- Modular design with clear separation of concerns
- Hardware abstraction layer for device independence
- Event-driven architecture
- Graceful signal handling and shutdown

## Building

### Prerequisites
- GCC/Clang with C++20 support
- CMake 3.16 or later
- libudev development libraries

### Build Command
```bash
mkdir -p build
cd build
cmake ..
cmake --build . --config Release
```

### Installation
```bash
sudo cmake --install .
```

## Development

### Project Structure
```
uccd/
├── main.cpp           # Application entry point and daemon initialization
├── CMakeLists.txt     # CMake build configuration
└── README.md          # This file
```

### Adding New Modules

Create new header/source files in the uccd directory and add them to CMakeLists.txt:

```cmake
add_executable(uccd
  main.cpp
  new_module.cpp
)
```

## Usage

### Run as daemon
```bash
sudo uccd --start
```

### Debug mode
```bash
uccd --debug
```

### Show version
```bash
tccd-ng --version
```

### Show help
```bash
tccd-ng --help
```

## Architecture Overview

### Main Components (Planned)

1. **Hardware Abstraction Layer**
   - Device detection and initialization
   - IOCTL interface to kernel drivers

2. **Service Modules**
   - Fan control
   - Power management
   - Display control
   - Webcam control
   - TDP management

3. **System Integration**
   - DBus interface
   - Configuration management
   - Logging and diagnostics

4. **Event Loop**
   - Asynchronous event handling
   - System signal management
   - Graceful shutdown

## License

GNU General Public License v3.0 - see LICENSE file in repository root

## Copyright

Copyright (c) 2024-2026 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
