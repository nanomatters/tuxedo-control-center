# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required( VERSION 3.20 )

project( ucc-tray
  VERSION 0.1.0
  DESCRIPTION "Uniwill Control Center system tray applet"
  LANGUAGES CXX
)

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set( CMAKE_AUTOMOC ON )

# Find Qt6
find_package( Qt6 REQUIRED COMPONENTS
  Core
  Gui
  Widgets
)

# Application sources
add_executable( ucc-tray
  main.cpp
)

target_include_directories( ucc-tray PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/libucc-dbus
)

target_link_libraries( ucc-tray PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  ucc-dbus
)

target_compile_options( ucc-tray PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
)

# Installation
install( TARGETS ucc-tray
  RUNTIME DESTINATION bin
)

# Desktop file for autostart
install( FILES ucc-tray.desktop
  DESTINATION share/applications
)
