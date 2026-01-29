# Quick Start - UCC Local Installation

## ✅ Setup Complete!

UCC is now configured for local installation at `~/tmp/ucc`

## 🚀 Quick Commands

### From VSCode
Press `Ctrl+Shift+P` → **Tasks: Run Task** → **UCC: Build, Install & Run**

### From Terminal
```bash
cd /opt/devel/git/tuxedo/tuxedo-control-center/ucc
./build-local.sh
cd build && cmake --install . && cd ..
./run-ucc-gui.sh
```

## 📁 Files Created

- **[build-local.sh](build-local.sh)** - Build script with ~/tmp/ucc install prefix
- **[run-ucc-gui.sh](run-ucc-gui.sh)** - Run script with environment setup
- **[INSTALL_LOCAL.md](INSTALL_LOCAL.md)** - Detailed installation guide
- **[UCC_SETUP_SUMMARY.md](UCC_SETUP_SUMMARY.md)** - Complete project summary

## 📋 VSCode Tasks Available

- `UCC: Build` - Build all components
- `UCC: Install` - Install to ~/tmp/ucc
- `UCC: Run GUI` - Launch the GUI application
- `UCC: Build, Install & Run` - Complete workflow

## 🎯 What is UCC?

Modern Qt6/KDE native application suite for Uniwill laptop control:
- **ucc-gui**: Main control center (QML UI)
- **ucc-tray**: System tray applet
- **ucc-widgets**: Plasma desktop widgets
- **libucc-dbus**: Shared DBus communication library

All components communicate with uccd daemon via DBus.

---

**Installation Path:** `~/tmp/src/ucc/`  
**Current Status:** ✅ Built and installed successfully
