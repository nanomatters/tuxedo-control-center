cmake_minimum_required(VERSION 3.16)
project(tccd-ng)

# Set default build type to RelWithDebInfo if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# Main executable
add_executable(tccd-ng
  tccd-ng.cpp
  TccDBusService.cpp
  workers/WebcamWorker.cpp
  profiles/FanProfiles.cpp
  profiles/TccProfile.cpp
  profiles/DefaultProfiles.cpp
)

# Include directories for tuxedo_io_lib
target_include_directories(tccd-ng PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../native-lib
)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDBUS REQUIRED IMPORTED_TARGET sdbus-c++)

# Link system libraries
target_link_libraries(tccd-ng PRIVATE
  udev
  PkgConfig::SDBUS
)

# Add compile options for stricter checking
target_compile_options(tccd-ng PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Wconversion
  -Wshadow
  -Wnon-virtual-dtor
  -Wold-style-cast
  -Wcast-align
  -Wunused
  -Woverloaded-virtual
  -Wdouble-promotion
  -Wformat=2
  -fexceptions
)

# Installation target
install(TARGETS tccd-ng
  RUNTIME DESTINATION bin
)
