/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <optional>
#include <memory>
#include <regex>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <filesystem>
#include <cstdio>
#include <functional>
#include "DaemonWorker.hpp"
#include "../SysfsNode.hpp"
#include "../Utils.hpp"

// Forward declarations
class AvailabilityService;

/**
 * @brief GPU Device Detection
 *
 * Detects available GPU devices by scanning /sys/bus/pci/devices
 * for vendor and product IDs. Supports Intel iGPU, AMD iGPU, AMD dGPU,
 * and NVIDIA dGPU detection.
 */
struct GpuDeviceCounts
{
  int intelIGpuCount = 0;
  int amdIGpuCount = 0;
  int amdDGpuCount = 0;
  int nvidiaCount = 0;
};

/**
 * @brief GPU Device Detector
 *
 * Provides functionality to detect available GPU devices by scanning
 * /sys/bus/pci/devices for vendor and product IDs.
 */
class GpuDeviceDetector
{
public:
  /**
   * @brief Constructor
   */
  explicit GpuDeviceDetector() noexcept = default;

  /**
   * @brief Detect all available GPU devices
   * @return GpuDeviceCounts with counts of each GPU type
   */
  [[nodiscard]] GpuDeviceCounts detectGpuDevices() const noexcept
  {
    GpuDeviceCounts counts;

    counts.intelIGpuCount = countDevicesMatchingPattern( getIntelIGpuPattern() );
    counts.amdIGpuCount = countDevicesMatchingPattern( getAmdIGpuPattern() );
    counts.amdDGpuCount = countDevicesMatchingPattern( getAmdDGpuPattern() );
    counts.nvidiaCount = countNvidiaDevices();

    return counts;
  }

private:
  // Intel iGPU Device IDs (vendor 8086)
  [[nodiscard]] std::string getIntelIGpuPattern() const noexcept
  {
    // Concatenate without pipe characters in the middle of string literals
    std::string pattern = "8086:(6420|64B0|7D51|7D67|7D41|7DD5|7D45|7D40|"
                          "A780|A781|A788|A789|A78A|A782|A78B|A783|A7A0|A7A1|A7A8|A7AA|"
                          "A7AB|A7AC|A7AD|A7A9|A721|4680|4690|4688|468A|468B|4682|4692|"
                          "4693|46D3|46D4|46D0|46D1|46D2|4626|4628|462A|46A2|46B3|46C2|"
                          "46A3|46B2|46C3|46A0|46B0|46C0|46A6|46AA|46A8)";
    return pattern;
  }

  // AMD iGPU Device IDs (vendor 1002)
  [[nodiscard]] std::string getAmdIGpuPattern() const noexcept
  {
    std::string pattern = "1002:(164E|1506|15DD|15D8|15E7|1636|1638|164C|164D|1681|15BF|"
                          "15C8|1304|1305|1306|1307|1309|130A|130B|130C|130D|130E|130F|"
                          "1310|1311|1312|1313|1315|1316|1317|1318|131B|131C|131D|13C0|"
                          "9830|9831|9832|9833|9834|9835|9836|9837|9838|9839|983a|983b|983c|"
                          "983d|983e|983f|9850|9851|9852|9853|9854|9855|9856|9857|9858|"
                          "9859|985A|985B|985C|985D|985E|985F|9870|9874|9875|9876|9877|"
                          "98E4|13FE|143F|74A0|1435|163f|1900|1901|1114|150E)";
    return pattern;
  }

  // AMD dGPU Device IDs (vendor 1002)
  [[nodiscard]] std::string getAmdDGpuPattern() const noexcept
  {
    return "1002:(7480)";
  }

  /**
   * @brief Count devices matching a vendor:device ID pattern
   *
   * Searches /sys/bus/pci/devices directory for matching vendor and device IDs.
   * Only counts devices with PCI_CLASS=30000 (Display controller / GPU).
   * This filters out audio devices (class 40300), network devices (class 20000), etc.
   *
   * @param pattern Regex pattern for vendor:device ID (e.g., "8086:4680")
   * @return Number of matching GPU devices
   */
  [[nodiscard]] int countDevicesMatchingPattern( const std::string &pattern ) const noexcept
  {
    try
    {
      // use grep to find devices matching the ID pattern AND having GPU class code
      // PCI_CLASS=30000 means Display controller (GPU)
      std::string command =
          "for f in /sys/bus/pci/devices/*/uevent; do "
          "grep -q 'PCI_CLASS=30000' \"$f\" && grep -q -P 'PCI_ID=" + pattern + "' \"$f\" && echo \"$f\"; "
          "done";
      
      std::string output = TccUtils::executeCommand( command );

      if ( output.empty() )
        return 0;

      // count lines in output
      int count = 0;
      for ( char c : output )
      {
        if ( c == '\n' )
          count++;
      }
      // account for last line without newline
      if ( not output.empty() and output.back() != '\n' )
        count++;

      return count;
    }
    catch ( const std::exception & )
    {
      return 0;
    }
  }

  /**
   * @brief Count NVIDIA devices by vendor ID
   *
   * Counts devices with NVIDIA vendor ID (0x10DE), accounting for
   * multiple functions of the same device. For example, "0000:01:00.0"
   * and "0000:01:00.1" belong to the same device "0000:01:00".
   *
   * @return Number of distinct NVIDIA devices
   */
  [[nodiscard]] int countNvidiaDevices() const noexcept
  {
    try
    {
      const std::string nvidiaVendorId = "0x10de";
      std::string command =
          "grep -lx '" + nvidiaVendorId + "' /sys/bus/pci/devices/*/vendor 2>/dev/null || echo ''";
      std::string output = TccUtils::executeCommand( command );

      if ( output.empty() )
        return 0;

      // extract unique device paths by removing function numbers
      // Path format: /sys/bus/pci/devices/0000:01:00.0/vendor
      // Device: 0000:01:00 (without the .0 or .1 or .2)
      std::set< std::string > uniqueDevices;
      std::istringstream iss( output );
      std::string line;

      while ( std::getline( iss, line ) )
      {
        if ( not line.empty() )
        {
          // extract the device path from /sys/bus/pci/devices/DEVICE/vendor
          // find the last occurrence of '/' and go back one more to get device path
          size_t lastSlash = line.rfind( '/' );
          if ( lastSlash != std::string::npos and lastSlash > 0 )
          {
            std::string devicePath = line.substr( 0, lastSlash );
            // remove function number (e.g., ".0" from "0000:01:00.0")
            size_t lastDot = devicePath.rfind( '.' );
            if ( lastDot != std::string::npos )
              devicePath = devicePath.substr( 0, lastDot );

            uniqueDevices.insert( devicePath );
          }
        }
      }

      return static_cast< int >( uniqueDevices.size() );
    }
    catch ( const std::exception & )
    {
      return 0;
    }
  }
};

/**
 * @brief Data structure for discrete GPU information
 */
struct DGpuInfo
{
  double m_temp = -1.0;
  double m_coreFrequency = -1.0;
  double m_maxCoreFrequency = -1.0;
  double m_powerDraw = -1.0;
  double m_maxPowerLimit = -1.0;
  double m_enforcedPowerLimit = -1.0;
  bool m_d0MetricsUsage = false;

  void print( void ) const noexcept
  {
    std::cout << "DGPU Info: \n"
       << "  Temperature: " << m_temp << " °C\n"
       << "  Core Frequency: " << m_coreFrequency << " MHz\n"
       << "  Max Core Frequency: " << m_maxCoreFrequency << " MHz\n"
       << "  Power Draw: " << m_powerDraw << " W\n"
       << "  Max Power Limit: " << m_maxPowerLimit << " W\n"
       << "  Enforced Power Limit: " << m_enforcedPowerLimit << " W\n"
       << "  D0 Metrics Usage: " << ( m_d0MetricsUsage ? "Yes" : "No" ) << "\n";
  }
};

/**
 * @brief Data structure for integrated GPU information
 */
struct IGpuInfo
{
  double m_temp = -1.0;
  double m_coreFrequency = -1.0;
  double m_maxCoreFrequency = -1.0;
  double m_powerDraw = -1.0;
  std::string m_vendor = "unknown";

  void print( void ) const noexcept
  {
    std::cout << "IGPU Info: \n"
       << "  Vendor: " << m_vendor << "\n"
       << "  Temperature: " << m_temp << " °C\n"
       << "  Core Frequency: " << m_coreFrequency << " MHz\n"
       << "  Max Core Frequency: " << m_maxCoreFrequency << " MHz\n"
       << "  Power Draw: " << m_powerDraw << " W\n";
  }
};

/**
 * @brief Intel RAPL (Running Average Power Limit) controller
 *
 * Provides access to Intel CPU power metrics via sysfs.
 * Manages energy counters and power constraints for CPU power management.
 *
 * Structure:
 * - basePath/name - RAPL device name (should be "package-0")
 * - basePath/enabled - Enable/disable flag
 * - basePath/energy_uj - Current energy counter in microjoules
 * - basePath/constraint_X_name - Constraint name (long_term, short_term, peak_power)
 * - basePath/constraint_X_max_power_uw - Maximum power in microwatts
 * - basePath/constraint_X_power_limit_uw - Power limit in microwatts
 */
class IntelRAPLController
{
public:
  /**
   * @brief Constructor
   * @param basePath Base path to Intel RAPL sysfs directory
   */
  explicit IntelRAPLController( const std::string &basePath ) noexcept
    : m_basePath( basePath ),
      m_energyUJ( 0 ),
      m_enabled( false ),
      m_name( "unknown" ),
      m_constraint0Name( "unknown" ),
      m_constraint0MaxPower( -1 ),
      m_constraint0PowerLimit( -1 ),
      m_constraint1Name( "unknown" ),
      m_constraint1MaxPower( -1 ),
      m_constraint1PowerLimit( -1 ),
      m_constraint2Name( "unknown" ),
      m_constraint2MaxPower( -1 ),
      m_constraint2PowerLimit( -1 )
  {
  }

  /**
   * @brief Check if CPU supports necessary Intel RAPL variables
   * @return True if RAPL power is available and device is "package-0"
   */
  [[nodiscard]] bool getIntelRAPLPowerAvailable() const noexcept
  {
    return m_enabled and m_name == "package-0" and m_energyUJ >= 0;
  }

  /**
   * @brief Check if constraint 0 (long_term) is available
   * @return True if long_term constraint is available
   */
  [[nodiscard]] bool getIntelRAPLConstraint0Available() const noexcept
  {
    return m_constraint0Name == "long_term" and m_constraint0MaxPower >= 0 and
           m_constraint0PowerLimit >= 0;
  }

  /**
   * @brief Check if constraint 1 (short_term) is available
   * @return True if short_term constraint is available
   */
  [[nodiscard]] bool getIntelRAPLConstraint1Available() const noexcept
  {
    return m_constraint1Name == "short_term" and m_constraint1MaxPower >= 0 and
           m_constraint1PowerLimit >= 0;
  }

  /**
   * @brief Check if constraint 2 (peak_power) is available
   * @return True if peak_power constraint is available
   */
  [[nodiscard]] bool getIntelRAPLConstraint2Available() const noexcept
  {
    return m_constraint2Name == "peak_power" and m_constraint2MaxPower >= 0 and
           m_constraint2PowerLimit >= 0;
  }

  /**
   * @brief Check if energyUJ is available
   * @return True if energy counter is available
   */
  [[nodiscard]] bool getIntelRAPLEnergyAvailable() const noexcept
  {
    return m_energyUJ >= 0;
  }

  /**
   * @brief Get the maximum value for constraint 0 power limit
   * @return Maximum power in microwatts or -1 if unavailable
   */
  [[nodiscard]] int64_t getConstraint0MaxPower() const noexcept
  {
    return m_constraint0PowerLimit;
  }

  /**
   * @brief Get the maximum value for constraint 1 power limit
   * @return Maximum power in microwatts or -1 if unavailable
   */
  [[nodiscard]] int64_t getConstraint1MaxPower() const noexcept
  {
    return m_constraint1PowerLimit;
  }

  /**
   * @brief Get the maximum value for constraint 2 power limit
   * @return Maximum power in microwatts or -1 if unavailable
   */
  [[nodiscard]] int64_t getConstraint2MaxPower() const noexcept
  {
    return m_constraint2PowerLimit;
  }

  /**
   * @brief Get the current energy counter in micro joules
   * @return Energy value in microjoules or -1 if unavailable
   *
   * NOTE: This reads from sysfs on every call to get the latest value,
   * matching the TypeScript implementation behavior.
   */
  [[nodiscard]] int64_t getEnergy() const noexcept
  {
    return readIntegerProperty( "energy_uj", -1 );
  }

  /**
   * @brief Set long term power limit (constraint 0)
   *
   * Sets the power limit for the CPU. If no value is provided, sets to maximum.
   * Value is automatically clamped to range [maxPower/2, maxPower].
   *
   * @param setPowerLimit Power limit in microwatts (optional)
   */
  void setPowerPL1Limit( std::optional< int64_t > setPowerLimit = std::nullopt ) noexcept
  {
    if ( not getIntelRAPLConstraint0Available() )
    {
      return;
    }

    int64_t maxPower = getConstraint0MaxPower();
    int64_t powerLimit = maxPower;

    if ( setPowerLimit.has_value() )
    {
      // clamp to range [maxPower/2, maxPower]
      powerLimit = std::max( maxPower / 2, std::min( setPowerLimit.value(), maxPower ) );
    }

    // write to sysfs
    if ( writeIntegerProperty( "constraint_0_power_limit_uw", powerLimit ) )
    {
      m_constraint0PowerLimit = powerLimit;
    }

    // enable the constraint
    writeBoolProperty( "enabled", true );
  }

  /**
   * @brief Update name from sysfs
   * @param name Device name to set
   */
  void setName( const std::string &name ) noexcept
  {
    m_name = name;
  }

  /**
   * @brief Update enabled state from sysfs
   * @param enabled State to set
   */
  void setEnabled( bool enabled ) noexcept
  {
    m_enabled = enabled;
  }

  /**
   * @brief Update energy reading from sysfs
   * @param energyValue Energy in microjoules
   */
  void setEnergy( int64_t energyValue ) noexcept
  {
    m_energyUJ = energyValue;
  }

  /**
   * @brief Update constraint 0 information
   * @param name Constraint name
   * @param maxPower Maximum power in microwatts
   * @param powerLimit Current power limit in microwatts
   */
  void setConstraint0( const std::string &name, int64_t maxPower, int64_t powerLimit ) noexcept
  {
    m_constraint0Name = name;
    m_constraint0MaxPower = maxPower;
    m_constraint0PowerLimit = powerLimit;
  }

  /**
   * @brief Update constraint 1 information
   * @param name Constraint name
   * @param maxPower Maximum power in microwatts
   * @param powerLimit Current power limit in microwatts
   */
  void setConstraint1( const std::string &name, int64_t maxPower, int64_t powerLimit ) noexcept
  {
    m_constraint1Name = name;
    m_constraint1MaxPower = maxPower;
    m_constraint1PowerLimit = powerLimit;
  }

  /**
   * @brief Update constraint 2 information
   * @param name Constraint name
   * @param maxPower Maximum power in microwatts
   * @param powerLimit Current power limit in microwatts
   */
  void setConstraint2( const std::string &name, int64_t maxPower, int64_t powerLimit ) noexcept
  {
    m_constraint2Name = name;
    m_constraint2MaxPower = maxPower;
    m_constraint2PowerLimit = powerLimit;
  }

  /**
   * @brief Get base path
   * @return Reference to base sysfs path
   */
  [[nodiscard]] const std::string &getBasePath() const noexcept
  {
    return m_basePath;
  }

  /**
   * @brief Refresh all RAPL values from sysfs
   *
   * Reads current values from the sysfs interface. This should be called
   * periodically to update the controller state.
   */
  void updateFromSysfs() noexcept
  {
    // update basic properties
    m_name = readStringProperty( "name", "unknown" );
    m_enabled = readBoolProperty( "enabled", false );
    m_energyUJ = readIntegerProperty( "energy_uj", -1 );

    // update constraint 0
    std::string c0Name = readStringProperty( "constraint_0_name", "unknown" );
    int64_t c0MaxPower = readIntegerProperty( "constraint_0_max_power_uw", -1 );
    int64_t c0PowerLimit = readIntegerProperty( "constraint_0_power_limit_uw", -1 );
    setConstraint0( c0Name, c0MaxPower, c0PowerLimit );

    // update constraint 1
    std::string c1Name = readStringProperty( "constraint_1_name", "unknown" );
    int64_t c1MaxPower = readIntegerProperty( "constraint_1_max_power_uw", -1 );
    int64_t c1PowerLimit = readIntegerProperty( "constraint_1_power_limit_uw", -1 );
    setConstraint1( c1Name, c1MaxPower, c1PowerLimit );

    // update constraint 2
    std::string c2Name = readStringProperty( "constraint_2_name", "unknown" );
    int64_t c2MaxPower = readIntegerProperty( "constraint_2_max_power_uw", -1 );
    int64_t c2PowerLimit = readIntegerProperty( "constraint_2_power_limit_uw", -1 );
    setConstraint2( c2Name, c2MaxPower, c2PowerLimit );
  }

private:
  /**
   * @brief Check if a sysfs property file exists and is readable
   * @param propertyName Name of the property file
   * @return True if file exists and is readable
   */
  [[nodiscard]] bool isPropertyAvailable( const std::string &propertyName ) const noexcept
  {
    try
    {
      std::string filePath = m_basePath + "/" + propertyName;
      return std::filesystem::exists( filePath ) and std::filesystem::is_regular_file( filePath );
    }
    catch ( const std::exception & )
    {
      return false;
    }
  }

  /**
   * @brief Read a string value from a sysfs property
   * @param propertyName Name of the property file
   * @param defaultValue Value to return if reading fails
   * @return Property value or defaultValue on error
   */
  [[nodiscard]] std::string readStringProperty( const std::string &propertyName,
                                               const std::string &defaultValue ) const noexcept
  {
    try
    {
      if ( not isPropertyAvailable( propertyName ) )
      {
        return defaultValue;
      }

      std::string filePath = m_basePath + "/" + propertyName;
      std::ifstream file( filePath );

      if ( not file.is_open() )
      {
        return defaultValue;
      }

      std::string value;
      if ( std::getline( file, value ) )
      {
        // remove trailing whitespace and newlines
        while ( not value.empty() and ( value.back() == '\n' or value.back() == '\r' or
                                         value.back() == ' ' or value.back() == '\t' ) )
        {
          value.pop_back();
        }
        return value;
      }

      return defaultValue;
    }
    catch ( const std::exception & )
    {
      return defaultValue;
    }
  }

  /**
   * @brief Read an integer value from a sysfs property
   * @param propertyName Name of the property file
   * @param defaultValue Value to return if reading fails
   * @return Property value or defaultValue on error
   */
  [[nodiscard]] int64_t readIntegerProperty( const std::string &propertyName,
                                            int64_t defaultValue ) const noexcept
  {
    try
    {
      if ( not isPropertyAvailable( propertyName ) )
        return defaultValue;

      std::string filePath = m_basePath + "/" + propertyName;
      std::ifstream file( filePath );

      if ( not file.is_open() )
        return defaultValue;

      int64_t value = defaultValue;
      if ( file >> value )
        return value;

      return defaultValue;
    }
    catch ( const std::exception & )
    {
      return defaultValue;
    }
  }

  /**
   * @brief Read a boolean value from a sysfs property
   *
   * Treats any non-zero value as true, zero as false.
   *
   * @param propertyName Name of the property file
   * @param defaultValue Value to return if reading fails
   * @return Property value or defaultValue on error
   */
  [[nodiscard]] bool readBoolProperty( const std::string &propertyName,
                                      bool defaultValue ) const noexcept
  {
    int64_t value = readIntegerProperty( propertyName, defaultValue ? 1 : 0 );
    return value != 0;
  }

  /**
   * @brief Write a value to a sysfs property
   * @param propertyName Name of the property file
   * @param value Value to write
   * @return True if write succeeded, false otherwise
   */
  bool writeIntegerProperty( const std::string &propertyName, int64_t value ) noexcept
  {
    try
    {
      std::string filePath = m_basePath + "/" + propertyName;
      std::ofstream file( filePath );

      if ( not file.is_open() )
        return false;

      file << value;
      return file.good();
    }
    catch ( const std::exception & )
    {
      return false;
    }
  }

  /**
   * @brief Write a boolean value to a sysfs property
   * @param propertyName Name of the property file
   * @param value Value to write (true = 1, false = 0)
   * @return True if write succeeded, false otherwise
   */
  bool writeBoolProperty( const std::string &propertyName, bool value ) noexcept
  {
    return writeIntegerProperty( propertyName, value ? 1 : 0 );
  }
  std::string m_basePath;
  int64_t m_energyUJ;
  bool m_enabled;
  std::string m_name;

  std::string m_constraint0Name;
  int64_t m_constraint0MaxPower;
  int64_t m_constraint0PowerLimit;

  std::string m_constraint1Name;
  int64_t m_constraint1MaxPower;
  int64_t m_constraint1PowerLimit;

  std::string m_constraint2Name;
  int64_t m_constraint2MaxPower;
  int64_t m_constraint2PowerLimit;
};

/**
 * @brief Power controller for Intel GPU power metrics
 *
 * Tracks energy consumption and calculates current power draw
 */
class PowerController
{
public:
  /**
   * @brief Constructor
   * @param intelRAPL Reference to Intel RAPL controller
   */
  explicit PowerController( IntelRAPLController &intelRAPL ) noexcept
    : m_intelRAPL( intelRAPL )
    , m_currentEnergy( 0 ), m_lastUpdateTime( std::chrono::system_clock::now() )
    , m_raplPowerStatus( intelRAPL.getIntelRAPLEnergyAvailable() )
  {
  }

  /**
   * @brief Get current power draw in watts
   * @return Power value or -1 if unavailable
   */
  [[nodiscard]] double getCurrentPower() noexcept
  {
    if ( not m_raplPowerStatus )
      return -1.0;

    auto now = std::chrono::system_clock::now();
    int64_t currentEnergy = m_intelRAPL.getEnergy();
    int64_t energyIncrement = currentEnergy - m_currentEnergy;

    auto timeDiff = std::chrono::duration_cast< std::chrono::milliseconds >(
        now - m_lastUpdateTime ).count();
    double delaySec = timeDiff > 0 ? static_cast< double >( timeDiff ) / 1000.0 : -1.0;

    double powerDraw = -1.0;
    if ( delaySec > 0 and m_currentEnergy > 0 )
    {
      powerDraw = static_cast< double >( energyIncrement ) / delaySec / 1000000.0;
    }

    // Match TypeScript: accumulate energy increment (handles RAPL counter rollover)
    m_currentEnergy += energyIncrement;
    m_lastUpdateTime = now;

    return powerDraw;
  }

private:
  IntelRAPLController &m_intelRAPL;
  int64_t m_currentEnergy;
  std::chrono::system_clock::time_point m_lastUpdateTime;
  bool m_raplPowerStatus;
};

/**
 * @brief GPU Information Worker
 *
 * Collects and updates GPU information from Intel, AMD, and NVIDIA GPUs.
 * Extends DaemonWorker with periodic GPU metrics collection.
 *
 * Supports:
 * - Intel iGPU via RAPL energy counters
 * - AMD iGPU via hwmon sysfs interface
 * - AMD dGPU via hwmon sysfs interface
 * - NVIDIA dGPU via nvidia-smi command
 */
class GpuInfoWorker : public DaemonWorker
{
public:
  /**
   * @brief Callback function type for GPU data updates
   */
  using GpuDataCallback = std::function< void( const IGpuInfo &, const DGpuInfo & ) >;

  /**
   * @brief Constructor
   */
  explicit GpuInfoWorker( void ) noexcept
    : DaemonWorker( std::chrono::milliseconds( 800 ), false ),
      m_gpuDetector( std::make_unique< GpuDeviceDetector >() ),
      m_deviceCounts( m_gpuDetector->detectGpuDevices() ),
      m_isNvidiaSmiInstalled( m_deviceCounts.nvidiaCount > 0 and checkNvidiaSmiInstalledImpl() ),
      m_gpuDataCallback( nullptr ),
      m_hwmonIGpuRetryCount( 3 ),
      m_hwmonDGpuRetryCount( 3 ),
      m_intelRAPLGpu( "/sys/devices/virtual/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:1/" ),
      m_intelPowerWorker( nullptr )
  {
  }

  /**
   * @brief Virtual destructor
   */
  virtual ~GpuInfoWorker() = default;

  // Prevent copy and move
  GpuInfoWorker( const GpuInfoWorker & ) = delete;
  GpuInfoWorker( GpuInfoWorker && ) = delete;
  GpuInfoWorker &operator=( const GpuInfoWorker & ) = delete;
  GpuInfoWorker &operator=( GpuInfoWorker && ) = delete;

  /**
   * @brief Set callback for GPU data updates
   * @param callback Function to call when GPU data is updated
   */
  void setGpuDataCallback( GpuDataCallback callback ) noexcept
  {
    m_gpuDataCallback = callback;
  }

protected:
  /**
   * @brief Called once when worker starts
   */
  void onStart() override
  {
    // check for Intel iGPU availability and set up power controller
    if ( m_deviceCounts.intelIGpuCount == 1 and not m_intelIGpuDrmPath.has_value() )
    {
      if ( std::string intelPath = getIntelIGpuDrmPathImpl(); not intelPath.empty() )
      {
        m_intelIGpuDrmPath = intelPath;
        m_intelPowerWorker = std::make_unique< PowerController >( m_intelRAPLGpu );
      }
    }

    // check for AMD iGPU availability and discover hwmon path
    if ( m_deviceCounts.amdIGpuCount == 1 and m_hwmonIGpuRetryCount > 0 )
    {
      ( void ) checkAmdIGpuHwmonPath();
    }

    // check for AMD dGPU availability and discover hwmon path
    if ( m_deviceCounts.amdDGpuCount == 1 and m_hwmonDGpuRetryCount > 0 and not m_amdDGpuHwmonPath.has_value() )
    {
      ( void ) checkAmdDGpuHwmonPath();
    }
  }

  /**
   * @brief Called periodically to collect GPU information
   */
  void onWork() override
  {
    // retry AMD iGPU path discovery if not found yet
    if ( m_deviceCounts.amdIGpuCount == 1 and not m_amdIGpuHwmonPath.has_value() and m_hwmonIGpuRetryCount > 0 )
      ( void ) checkAmdIGpuHwmonPath();

    // retry AMD dGPU path discovery if not found yet
    if ( m_deviceCounts.amdDGpuCount == 1 and not m_amdDGpuHwmonPath.has_value() and m_hwmonDGpuRetryCount > 0 )
      ( void ) checkAmdDGpuHwmonPath();

    updateDaemonGpuData( getIGpuValues(), getDGpuValues() );
  }

  /**
   * @brief Called when worker exits
   */
  void onExit() override
  {
    // clean up any resources
  }

private:
  // GPU Detection
  std::unique_ptr< GpuDeviceDetector > m_gpuDetector;
  GpuDeviceCounts m_deviceCounts;

  // State variables
  bool m_isNvidiaSmiInstalled;
  GpuDataCallback m_gpuDataCallback;
  std::optional< std::string > m_amdIGpuHwmonPath;
  std::optional< std::string > m_amdDGpuHwmonPath;
  std::optional< std::string > m_intelIGpuDrmPath;

  int m_hwmonIGpuRetryCount;
  int m_hwmonDGpuRetryCount;

  IntelRAPLController m_intelRAPLGpu;
  std::unique_ptr< PowerController > m_intelPowerWorker;

  /**
   * @brief Get Intel iGPU DRM path via sysfs
   * @return Path string or empty string if not found
   */
  [[nodiscard]] std::string getIntelIGpuDrmPathImpl() const noexcept
  {
    // implementation would query sysfs for Intel GPU device path
    // grep -lP 'Intel GPU ID' /sys/bus/pci/devices/*/drm/card*/device/uevent
    return "";
  }

  /**
   * @brief Check if AMD iGPU hwmon path exists
   * @return True if path successfully discovered and set
   */
  [[nodiscard]] bool checkAmdIGpuHwmonPath() noexcept
  {
    if ( m_amdIGpuHwmonPath.has_value() )
      return true;

    if ( m_hwmonIGpuRetryCount > 0 )
    {
      m_hwmonIGpuRetryCount--;
      if ( std::string amdIGpuPath = getAmdIGpuHwmonPathImpl(); not amdIGpuPath.empty() )
      {
        m_amdIGpuHwmonPath = amdIGpuPath;
        return true;
      }
    }

    return false;
  }

  /**
   * @brief Discover AMD iGPU hwmon path via sysfs
   * @return Path to hwmon device or empty string if not found
   */
  [[nodiscard]] std::string getAmdIGpuHwmonPathImpl() const noexcept
  {
    try
    {
      // Get AMD iGPU device ID pattern
      std::string amdIGpuPattern = "1002:(164E|1506|15DD|15D8|15E7|1636|1638|164C|164D|1681|15BF|"
                                   "15C8|1304|1305|1306|1307|1309|130A|130B|130C|130D|130E|130F|"
                                   "1310|1311|1312|1313|1315|1316|1317|1318|131B|131C|131D|13C0|"
                                   "9830|9831|9832|9833|9834|9835|9836|9837|9838|9839|983a|983b|983c|"
                                   "983d|983e|983f|9850|9851|9852|9853|9854|9855|9856|9857|9858|"
                                   "9859|985A|985B|985C|985D|985E|985F|9870|9874|9875|9876|9877|"
                                   "98E4|13FE|143F|74A0|1435|163f|1900|1901|1114|150E)";

      // search for hwmon devices with amdgpu driver and matching PCI ID
      std::string command =
          "for d in /sys/class/hwmon/hwmon*/device/uevent; do "
          "grep -q 'DRIVER=amdgpu' \"$d\" && grep -q -P 'PCI_ID=" + amdIGpuPattern + "' \"$d\" && "
          "dirname \"$d\" | sed 's|/device$||'; "
          "done | head -1";

      std::string output = TccUtils::executeCommand( command );

      // trim whitespace
      while ( not output.empty() and ( output.back() == '\n' or output.back() == '\r' or
                                       output.back() == ' ' or output.back() == '\t' ) )
      {
        output.pop_back();
      }

      return output;
    }
    catch ( const std::exception & )
    {
      return "";
    }
  }

  /**
   * @brief Check if nvidia-smi tool is installed
   * @return True if nvidia-smi is available
   */
  [[nodiscard]] bool checkNvidiaSmiInstalledImpl() const noexcept
  {
    try
    {
      if ( std::string output = TccUtils::executeCommand( "which nvidia-smi" ); true )
      {
        // trim the output
        while ( not output.empty() and ( output.back() == '\n' or output.back() == '\r' or
                                         output.back() == ' ' or output.back() == '\t' ) )
        {
          output.pop_back();
        }

        return not output.empty();
      }
    }
    catch ( const std::exception & )
    {
      return false;
    }
  }

  /**
   * @brief Check if AMD dGPU hwmon path exists
   * @return True if path successfully discovered and set
   */
  [[nodiscard]] bool checkAmdDGpuHwmonPath() noexcept
  {
    if ( m_amdDGpuHwmonPath.has_value() )
      return true;

    if ( m_hwmonDGpuRetryCount > 0 )
    {
      m_hwmonDGpuRetryCount--;
      if ( std::string amdDGpuPath = getAmdDGpuHwmonPathImpl(); not amdDGpuPath.empty() )
      {
        m_amdDGpuHwmonPath = amdDGpuPath;
        return true;
      }
    }

    return false;
  }

  /**
   * @brief Discover AMD dGPU hwmon path via sysfs
   * @return Path to hwmon device or empty string if not found
   */
  [[nodiscard]] std::string getAmdDGpuHwmonPathImpl() const noexcept
  {
    // implementation would query sysfs for AMD dGPU hwmon path
    // grep -lP 'AMD_DGPU_ID' /sys/class/hwmon/*/device/uevent | sed 's|/device/uevent$||'
    return "";
  }

  /**
   * @brief Get Intel iGPU values
   * @param iGpuValues Reference to structure to fill with values
   * @return Updated IGpuInfo structure
   */
  [[nodiscard]] IGpuInfo getIntelIGpuValues( const IGpuInfo &iGpuValues ) const noexcept
  {
    IGpuInfo values = iGpuValues;

    if ( not m_intelIGpuDrmPath.has_value() )
      return values;

    // read core frequency from:
    // {drm_path}/gt_act_freq_mhz
    // {drm_path}/gt_RP0_freq_mhz

    if ( m_intelPowerWorker )
      values.m_powerDraw = m_intelPowerWorker->getCurrentPower();

    return values;
  }

  /**
   * @brief Get AMD iGPU values
   * @param iGpuValues Reference to structure to fill with values
   * @return Updated IGpuInfo structure
   */
  [[nodiscard]] IGpuInfo getAmdIGpuValues( const IGpuInfo &iGpuValues ) const noexcept
  {
    IGpuInfo values = iGpuValues;

    if ( not m_amdIGpuHwmonPath.has_value() )
      return values;

    std::string hwmonPath = m_amdIGpuHwmonPath.value();

    // set vendor
    values.m_vendor = "amd";

    // read temperature from {hwmon_path}/temp1_input (divide by 1000)
    if ( int64_t tempValue = SysfsNode< int64_t >( hwmonPath + "/temp1_input" ).read().value_or( -1 ); tempValue >= 0 )
      values.m_temp = static_cast< double >( tempValue ) / 1000.0;

    // read current frequency from {hwmon_path}/freq1_input (divide by 1000000)
    if ( int64_t curFreqValue = SysfsNode< int64_t >( hwmonPath + "/freq1_input" ).read().value_or( -1 ); curFreqValue >= 0 )
      values.m_coreFrequency = static_cast< double >( curFreqValue ) / 1000000.0;

    // read max frequency from {hwmon_path}/device/pp_dpm_sclk
    if ( std::string maxFreqString = SysfsNode< std::string >( hwmonPath + "/device/pp_dpm_sclk" ).read().value_or( "" ); not maxFreqString.empty() )
      values.m_maxCoreFrequency = parseMaxAmdFreq( maxFreqString );

    // read power from {hwmon_path}/power1_input (divide by 1000 to convert milliwatts to watts)
    if ( int64_t powerValue = SysfsNode< int64_t >( hwmonPath + "/power1_input" ).read().value_or( -1 ); powerValue >= 0 )
      values.m_powerDraw = static_cast< double >( powerValue ) / 1000.0;

    return values;
  }

  /**
   * @brief Parse AMD frequency string
   * @param frequencyString String containing MHz values
   * @return Maximum frequency value
   */
  [[nodiscard]] double parseMaxAmdFreq( const std::string &frequencyString ) const noexcept
  {
    std::regex mhzRegex( R"(\d+Mhz)" );
    std::smatch match;
    double maxFreq = -1.0;

    std::string::const_iterator searchStart( frequencyString.cbegin() );
    while ( std::regex_search( searchStart, frequencyString.cend(), match, mhzRegex ) )
    {
      try
      {
        std::string numStr = match.str();
        numStr = numStr.substr( 0, numStr.length() - 3 ); // Remove "Mhz"
        double freq = std::stod( numStr );
        if ( freq > maxFreq )
          maxFreq = freq;
      }
      catch ( const std::exception & )
      {
        // Ignore parsing errors
      }
      searchStart = match.suffix().first;
    }

    return maxFreq;
  }

  /**
   * @brief Get iGPU information
   *
   * Determines which iGPU is available and collects its data.
   * Supports Intel iGPU via RAPL power counters and DRM frequency,
   * or AMD iGPU via hwmon sysfs interface.
   *
   * @return Collected iGPU data
   */
  [[nodiscard]] IGpuInfo getIGpuValues() noexcept
  {
    IGpuInfo iGpuValues{};

    // Determine GPU vendor and collect appropriate data
    if ( m_intelIGpuDrmPath.has_value() )
    {
      iGpuValues = getIntelIGpuValues( iGpuValues );
    }
    else if ( m_amdIGpuHwmonPath.has_value() )
    {
      iGpuValues = getAmdIGpuValues( iGpuValues );
    }

    return iGpuValues;
  }

  /**
   * @brief Get NVIDIA dGPU values
   * @return Collected dGPU data
   */
  [[nodiscard]] DGpuInfo getNvidiaDGpuValues() const noexcept
  {
    if ( m_isNvidiaSmiInstalled )
    {
      // Execute nvidia-smi to get power and frequency information
      std::string command = "nvidia-smi --query-gpu=temperature.gpu,power.draw,power.max_limit,enforced.power.limit,clocks.gr,clocks.max.gr --format=csv,noheader";
      std::string output = TccUtils::executeCommand( command );

      if ( not output.empty() )
        return parseNvidiaOutput( output );
    }
 
    return DGpuInfo{};
  }

  /**
   * @brief Get AMD dGPU values
   * @param dGpuValues Reference to structure to fill with values
   * @return Updated DGpuInfo structure
   */
  [[nodiscard]] DGpuInfo getAmdDGpuValues( const DGpuInfo &dGpuValues ) const noexcept
  {
    DGpuInfo values = dGpuValues;

    if ( not m_amdDGpuHwmonPath.has_value() )
      return values;

    // Read temperature from:
    // {hwmon_path}/temp1_input (divide by 1000)

    // Read frequency from:
    // {hwmon_path}/freq1_input (divide by 1000000)

    // Read power from:
    // {hwmon_path}/power1_average (divide by 1000000)

    try
    {
      const std::string &base = m_amdDGpuHwmonPath.value();

      int64_t tempMilli = SysfsNode< int64_t >( base + "/temp1_input" ).read().value_or( -1000 );
      if ( tempMilli > -1000 )
        values.m_temp = static_cast< double >( tempMilli ) / 1000.0;

      int64_t freqHz = SysfsNode< int64_t >( base + "/freq1_input" ).read().value_or( -1 );
      if ( freqHz > 0 )
        values.m_coreFrequency = static_cast< double >( freqHz ) / 1000000.0;

      int64_t powerMicro = SysfsNode< int64_t >( base + "/power1_average" ).read().value_or( -1 );
      if ( powerMicro > 0 )
        values.m_powerDraw = static_cast< double >( powerMicro ) / 1000000.0;
    }
    catch ( ... )
    {
    }

    return values;
  }

  /**
   * @brief Get dGPU information
   *
   * Collects discrete GPU data based on availability and metrics usage setting.
   * Supports NVIDIA dGPU via nvidia-smi and AMD dGPU via hwmon sysfs interface.
   *
   * @return Collected dGPU data
   */
  [[nodiscard]] DGpuInfo getDGpuValues() noexcept
  {
    DGpuInfo dGpuValues{};

    // Get metrics usage status from daemon
    // Note: In actual implementation, this would be retrieved from m_tccd.dbusData.d0MetricsUsage
    bool metricsUsage = true; // Placeholder - should be retrieved from daemon

    // Check if NVIDIA dGPU is available and metrics are enabled
    if ( m_deviceCounts.nvidiaCount == 1 and m_isNvidiaSmiInstalled and metricsUsage )
    {
      dGpuValues = getNvidiaDGpuValues();
    }
    // Check if AMD dGPU is available and metrics are enabled
    else if ( m_deviceCounts.amdDGpuCount == 1 and metricsUsage )
    {
      if ( m_amdDGpuHwmonPath.has_value() or checkAmdDGpuHwmonPath() )
      {
        dGpuValues = getAmdDGpuValues( dGpuValues );
      }
    }

    // Set metrics usage flag
    dGpuValues.m_d0MetricsUsage = metricsUsage;

    return dGpuValues;
  }

  /**
   * @brief Parse NVIDIA output format
   *
   * Parses comma-separated values from nvidia-smi output.
  * Format: temperature.gpu, power.draw, power.max_limit, enforced.power.limit, clocks.gr, clocks.max.gr
  * Example: "65, 100.00 W, 150.00 W, 150.00 W, 1500 MHz, 2000 MHz"
   *
   * @param output Command output string
   * @return DGpuInfo with parsed values
   */
  [[nodiscard]] DGpuInfo parseNvidiaOutput( const std::string &output ) const noexcept
  {
    DGpuInfo values{};

    // Split output by comma
    std::vector< std::string > fields;
    std::stringstream ss( output );
    std::string field;

    while ( std::getline( ss, field, ',' ) )
    {
      // Trim leading/trailing whitespace
      size_t start = field.find_first_not_of( " \t\n\r" );
      size_t end = field.find_last_not_of( " \t\n\r" );
      if ( start != std::string::npos )
        fields.push_back( field.substr( start, end - start + 1 ) );
    }

    // Expected: [temp, powerDraw, maxPowerLimit, enforcedPowerLimit, coreFrequency, maxCoreFrequency]
    if ( fields.size() >= 6 )
    {
      values.m_temp = parseNumberWithMetric( fields[ 0 ] );
      values.m_powerDraw = parseNumberWithMetric( fields[ 1 ] );
      values.m_maxPowerLimit = parseNumberWithMetric( fields[ 2 ] );
      values.m_enforcedPowerLimit = parseNumberWithMetric( fields[ 3 ] );
      values.m_coreFrequency = parseNumberWithMetric( fields[ 4 ] );
      values.m_maxCoreFrequency = parseNumberWithMetric( fields[ 5 ] );
    }

    return values;
  }

  /**
   * @brief Parse number with optional metric suffix
   * @param value String containing number and optional unit
   * @return Parsed numeric value
   */
  [[nodiscard]] double parseNumberWithMetric( const std::string &value ) const noexcept
  {
    std::regex numberRegex( R"(\d+(\.\d+)?)" );
    std::smatch match;

    if ( std::regex_search( value, match, numberRegex ) )
    {
      try
      {
        return std::stod( match.str() );
      }
      catch ( const std::exception & )
      {
        return -1.0;
      }
    }

    return -1.0;
  }

  /**
   * @brief Update daemon with collected GPU data
   * @param iGpuValues Integrated GPU information
   * @param dGpuValues Discrete GPU information
   */
  void updateDaemonGpuData( const IGpuInfo &iGpuValues, const DGpuInfo &dGpuValues ) noexcept
  {
    try
    {
      if ( m_gpuDataCallback )
        m_gpuDataCallback( iGpuValues, dGpuValues );
    }
    catch ( ... )
    {
      // ignore any exceptions from callback
    }
  }
};
