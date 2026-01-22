# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required( VERSION 3.20 )

project( ucc-gui
  VERSION 0.1.0
  DESCRIPTION "Uniwill Control Center GUI application"
  LANGUAGES CXX
)

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( CMAKE_AUTOUIC ON )

# Find Qt6
find_package( Qt6 REQUIRED COMPONENTS
  Core
  Gui
  Qml
  Quick
  QuickControls2
)

# Application sources
add_executable( ucc-gui
  main.cpp
  ProfileManager.cpp
  SystemMonitor.cpp
  resources.qrc
)

target_include_directories( ucc-gui PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/libucc-dbus
)

target_link_libraries( ucc-gui PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
  Qt6::QuickControls2
  ucc-dbus
)

target_compile_options( ucc-gui PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
)

# Installation
install( TARGETS ucc-gui
  RUNTIME DESTINATION bin
)
