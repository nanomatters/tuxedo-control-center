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

#include "DaemonWorker.hpp"
#include "../SysfsNode.hpp"
#include "../Utils.hpp"
#include "../profiles/UccProfile.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <thread>

/**
 * @brief Controller for a single display backlight driver
 * 
 * Handles reading and writing brightness values via sysfs.
 * Includes workaround for amdgpu_bl driver which reports values in [0, 0xffff] range.
 */
class DisplayBacklightController
{
public:
  explicit DisplayBacklightController( const std::string &basePath, const std::string &driverName )
    : m_basePath( basePath ),
      m_driver( driverName ),
      m_brightnessPath( basePath + "/" + driverName + "/brightness" ),
      m_actualBrightnessPath( basePath + "/" + driverName + "/actual_brightness" ),
      m_maxBrightnessPath( basePath + "/" + driverName + "/max_brightness" ),
      m_isAmdgpuDriver( driverName.find( "amdgpu_bl" ) not_eq std::string::npos )
  {
  }

  /**
   * @brief Read current brightness value
   * @return Current brightness (0-255 range, scaled if amdgpu)
   */
  [[nodiscard]] int32_t readBrightness() const noexcept
  {
    int32_t value = SysfsNode< int32_t >( m_actualBrightnessPath ).read().value_or( -1 );
    
    // amdgpu brightness workaround: scale [0, 0xffff] to [0, 0xff]
    if ( m_isAmdgpuDriver and value > 0xff )
      value = static_cast< int32_t >( std::round( 0xff * ( static_cast< double >( value ) / 0xffff ) ) );
    
    return value;
  }

  /**
   * @brief Write brightness value
   * @param value Brightness value to write
   * @return true on success
   */
  [[nodiscard]] bool writeBrightness( int32_t value ) const noexcept
  {
    return SysfsNode< int32_t >( m_brightnessPath ).write( value );
  }

  /**
   * @brief Read maximum brightness value
   * @return Maximum brightness value
   */
  [[nodiscard]] int32_t readMaxBrightness() const noexcept
  {
    return SysfsNode< int32_t >( m_maxBrightnessPath ).read().value_or( -1 );
  }

  [[nodiscard]] const std::string &getDriver() const noexcept { return m_driver; }

private:
  std::string m_basePath;
  std::string m_driver;
  std::string m_brightnessPath;
  std::string m_actualBrightnessPath;
  std::string m_maxBrightnessPath;
  bool m_isAmdgpuDriver;
};

/**
 * @brief Worker for managing display backlight brightness
 * 
 * Applies brightness settings from active profile and saves current brightness
 * to autosave file. Matches TypeScript DisplayBacklightWorker functionality.
 */
class DisplayBacklightWorker : public DaemonWorker
{
public:
  explicit DisplayBacklightWorker( 
    const std::string &autosavePath,
    std::function< UccProfile() > getActiveProfile,
    std::function< int32_t() > getAutosaveBrightness,
    std::function< void( int32_t ) > setAutosaveBrightness )
    : DaemonWorker( std::chrono::milliseconds( 3000 ), false ),
      m_basePath( "/sys/class/backlight" ),
      m_autosavePath( autosavePath ),
      m_getActiveProfile( getActiveProfile ),
      m_getAutosaveBrightness( getAutosaveBrightness ),
      m_setAutosaveBrightness( setAutosaveBrightness )
  {
  }

protected:
  void onStart() override
  {
    // Figure out which brightness percentage to set
    const UccProfile currentProfile = m_getActiveProfile();
    
    if ( currentProfile.id.empty() )
      return;
    
    int32_t brightnessPercent;
    
    if ( not currentProfile.display.useBrightness or currentProfile.display.brightness < 0 )
    {
      const int32_t autosaveBrightness = m_getAutosaveBrightness();

      if ( autosaveBrightness < 0 )
        brightnessPercent = 100;
      else
        brightnessPercent = autosaveBrightness;
    }
    else
      brightnessPercent = currentProfile.display.brightness;

    // Write brightness percentage to driver(s)
    if ( currentProfile.display.useBrightness )
    {
      writeBrightness( brightnessPercent );

      // Recheck workaround for late loaded drivers and drivers that are not ready
      // although already presenting an interface
      std::thread( [this, brightnessPercent]()
      {
        std::this_thread::sleep_for( std::chrono::milliseconds( 2000 ) );
        writeBrightness( brightnessPercent, true );
      } ).detach();
    }
  }

  void onWork() override
  {
    // Reenumerate drivers (they can change on the fly)
    findDrivers();

    // Possibly save brightness regularly
    for ( const auto &controller : m_controllers )
    {
      const int32_t value = controller.readBrightness();
      const int32_t maxBrightness = controller.readMaxBrightness();
      
      if ( value > 0 and maxBrightness > 0 )
      {
        const int32_t brightnessPercent = static_cast< int32_t >( std::round( ( value * 100.0 ) / maxBrightness ) );
        m_setAutosaveBrightness( brightnessPercent );
      }
    }
  }

  void onExit() override
  {
    // Reenumerate drivers before exit
    findDrivers();

    for ( const auto &controller : m_controllers )
    {
      const int32_t value = controller.readBrightness();
      const int32_t maxBrightness = controller.readMaxBrightness();
      
      if ( value < 0 or maxBrightness < 0 )
      {
        std::cerr << "[DisplayBacklightWorker] Failed to read display brightness on exit from "
                  << controller.getDriver() << std::endl;
        continue;
      }

      if ( value == 0 )
        std::cout << "[DisplayBacklightWorker] Refused to save display brightness 0 from "
                  << controller.getDriver() << std::endl;
      else
      {
        const int32_t brightnessPercent = static_cast< int32_t >( std::round( ( value * 100.0 ) / maxBrightness ) );
        m_setAutosaveBrightness( brightnessPercent );
        std::cout << "[DisplayBacklightWorker] Save display brightness "
                  << brightnessPercent << "% (" << value << ") on exit" << std::endl;
      }
    }
  }

private:
  std::string m_basePath;
  std::string m_autosavePath;
  std::vector< DisplayBacklightController > m_controllers;
  std::function< UccProfile() > m_getActiveProfile;
  std::function< int32_t() > m_getAutosaveBrightness;
  std::function< void( int32_t ) > m_setAutosaveBrightness;

  /**
   * @brief Find and update list of available backlight drivers
   */
  void findDrivers() noexcept
  {
    m_controllers.clear();
    
    const auto driverNames = TccUtils::getDeviceList( m_basePath );

    for ( const auto &driverName : driverNames )
      m_controllers.emplace_back( m_basePath, driverName );
  }

  /**
   * @brief Write brightness percentage to all available drivers
   * @param brightnessPercent Brightness percentage (0-100)
   * @param recheck If true, only write if value differs from current
   */
  void writeBrightness( int32_t brightnessPercent, bool recheck = false ) noexcept
  {
    findDrivers();
    
    // try all possible drivers to be on the safe side
    for ( const auto &controller : m_controllers )
    {
      const int32_t maxBrightness = controller.readMaxBrightness();

      if ( maxBrightness < 0 )
        continue;

      const int32_t currentBrightnessRaw = controller.readBrightness();
      const int32_t brightnessRaw = static_cast< int32_t >( std::round( ( brightnessPercent * maxBrightness ) / 100.0 ) );

      if ( recheck and currentBrightnessRaw == brightnessRaw )
        continue; // value already correct

      if ( recheck and currentBrightnessRaw not_eq brightnessRaw )
        std::cout << "[DisplayBacklightWorker] Brightness not as expected for "
                  << controller.getDriver() << ", applying value again.." << std::endl;

      if ( not recheck )
        std::cout << "[DisplayBacklightWorker] Set display brightness to "
                  << brightnessPercent << "% (" << brightnessRaw << ") on "
                  << controller.getDriver() << std::endl;

      if ( not controller.writeBrightness( brightnessRaw ) )
        std::cerr << "[DisplayBacklightWorker] Failed to set display brightness to "
                  << brightnessPercent << "% (" << brightnessRaw << ") on "
                  << controller.getDriver() << std::endl;
    }
  }
};
