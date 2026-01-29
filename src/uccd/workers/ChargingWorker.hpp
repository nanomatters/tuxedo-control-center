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
#include "../PowerSupplyController.hpp"
#include "../SysfsNode.hpp"
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <sstream>
#include <syslog.h>

/**
 * @brief ChargingWorker - Battery charging control worker
 *
 * Manages:
 * - Battery charging thresholds (start/end)
 * - Charging profiles (high_capacity, balanced, stationary)
 * - Charging priorities
 * - Charge type (Standard, Custom, etc.)
 *
 * Sysfs paths:
 * - /sys/class/power_supply/BAT0/charge_control_start_threshold
 * - /sys/class/power_supply/BAT0/charge_control_end_threshold
 * - /sys/class/power_supply/BAT0/charge_type
 * - /sys/devices/platform/tuxedo_keyboard/charging_profile
 * - /sys/devices/platform/tuxedo_keyboard/charging_profiles_available
 * - /sys/devices/platform/tuxedo_keyboard/charging_prio
 * - /sys/devices/platform/tuxedo_keyboard/charging_prios_available
 */
class ChargingWorker : public DaemonWorker
{
public:
  ChargingWorker()
    : DaemonWorker( std::chrono::milliseconds( 10000 ), false ),
      m_basePath( "/sys/devices/platform/tuxedo_keyboard/charging_profile" )
  {
  }

  void onStart() override
  {
    // Initialize settings from current hardware state
    initializeSettings();
  }

  void onWork() override
  {
    // Periodic checks if needed (currently no periodic work required)
  }

  void onExit() override
  {
  }

  // === Charging Profile Methods ===

  bool hasChargingProfile() const noexcept
  {
    return SysfsNode< std::string >( m_basePath + "/charging_profile" ).isAvailable() and
           SysfsNode< std::string >( m_basePath + "/charging_profiles_available" ).isAvailable();
  }

  std::vector< std::string > getChargingProfilesAvailable() const noexcept
  {
    if ( not hasChargingProfile() )
      return {};

    auto profiles = SysfsNode< std::vector< std::string > >( m_basePath + "/charging_profiles_available", " " ).read();
    return profiles.value_or( std::vector< std::string >{} );
  }

  std::string getCurrentChargingProfile() const noexcept
  {
    if ( m_currentChargingProfile.empty() )
      return "";
    
    return m_currentChargingProfile;
  }

  bool applyChargingProfile( const std::string &profileDescriptor ) noexcept
  {
    if ( not hasChargingProfile() )
      return false;

    // Update saved setting
    if ( not profileDescriptor.empty() )
      m_currentChargingProfile = profileDescriptor;

    try
    {
      const std::string profileToSet = m_currentChargingProfile;
      const std::string currentProfile = SysfsNode< std::string >( m_basePath + "/charging_profile" ).read().value_or( "" );
      const auto profilesAvailable = getChargingProfilesAvailable();

      // Check if profile is in available list
      auto it = std::find( profilesAvailable.begin(), profilesAvailable.end(), profileToSet );
      
      if ( not profileToSet.empty() and profileToSet != currentProfile and it != profilesAvailable.end() )
      {
        if ( SysfsNode< std::string >( m_basePath + "/charging_profile" ).write( profileToSet ) )
        {
          syslog( LOG_INFO, "Applied charging profile '%s'", profileToSet.c_str() );
          return true;
        }
      }
    }
    catch ( ... )
    {
      syslog( LOG_WARNING, "Failed applying charging profile" );
    }

    return false;
  }

  // === Charging Priority Methods ===

  bool hasChargingPriority() const noexcept
  {
    // Charging priority is in a different subdirectory
    return SysfsNode< std::string >( "/sys/devices/platform/tuxedo_keyboard/charging_priority/charging_prio" ).isAvailable() and
           SysfsNode< std::string >( "/sys/devices/platform/tuxedo_keyboard/charging_priority/charging_prios_available" ).isAvailable();
  }

  std::vector< std::string > getChargingPrioritiesAvailable() const noexcept
  {
    if ( not hasChargingPriority() )
      return {};

    auto prios = SysfsNode< std::vector< std::string > >( "/sys/devices/platform/tuxedo_keyboard/charging_priority/charging_prios_available", " " ).read();
    return prios.value_or( std::vector< std::string >{} );
  }

  std::string getCurrentChargingPriority() const noexcept
  {
    if ( m_currentChargingPriority.empty() )
      return "";
    
    return m_currentChargingPriority;
  }

  bool applyChargingPriority( const std::string &priorityDescriptor ) noexcept
  {
    if ( not hasChargingPriority() )
      return false;

    // Update saved setting
    if ( not priorityDescriptor.empty() )
      m_currentChargingPriority = priorityDescriptor;

    try
    {
      const std::string prioToSet = m_currentChargingPriority;
      const std::string currentPrio = SysfsNode< std::string >( "/sys/devices/platform/tuxedo_keyboard/charging_priority/charging_prio" ).read().value_or( "" );
      const auto priosAvailable = getChargingPrioritiesAvailable();

      // Check if priority is in available list
      auto it = std::find( priosAvailable.begin(), priosAvailable.end(), prioToSet );
      
      if ( not prioToSet.empty() and prioToSet != currentPrio and it != priosAvailable.end() )
      {
        if ( SysfsNode< std::string >( "/sys/devices/platform/tuxedo_keyboard/charging_priority/charging_prio" ).write( prioToSet ) )
        {
          syslog( LOG_INFO, "Applied charging priority '%s'", prioToSet.c_str() );
          return true;
        }
      }
    }
    catch ( ... )
    {
      syslog( LOG_WARNING, "Failed applying charging priority" );
    }

    return false;
  }

  // === Charge Threshold Methods ===

  std::vector< int > getChargeStartAvailableThresholds() const noexcept
  {
    auto battery = PowerSupplyController::getFirstBattery();
    if ( not battery )
      return {};

    // Check if start threshold is available
    if ( battery->getChargeControlStartThreshold() == -1 )
      return {};

    // Try to read available thresholds list
    auto thresholds = battery->getChargeControlStartAvailableThresholds();
    
    // If empty, default to 0-100
    if ( thresholds.empty() )
    {
      thresholds.resize( 101 );
      for ( size_t i = 0; i <= 100; ++i )
        thresholds[ i ] = static_cast< int >( i );
    }

    return thresholds;
  }

  std::vector< int > getChargeEndAvailableThresholds() const noexcept
  {
    auto battery = PowerSupplyController::getFirstBattery();
    if ( not battery )
      return {};

    // Check if end threshold is available
    if ( battery->getChargeControlEndThreshold() == -1 )
      return {};

    // Try to read available thresholds list
    auto thresholds = battery->getChargeControlEndAvailableThresholds();
    
    // If empty, default to 0-100
    if ( thresholds.empty() )
    {
      thresholds.resize( 101 );
      for ( size_t i = 0; i <= 100; ++i )
        thresholds[ i ] = static_cast< int >( i );
    }

    return thresholds;
  }

  int getChargeStartThreshold() const noexcept
  {
    auto battery = PowerSupplyController::getFirstBattery();
    if ( not battery )
      return -1;

    return battery->getChargeControlStartThreshold();
  }

  bool setChargeStartThreshold( int value ) noexcept
  {
    auto battery = PowerSupplyController::getFirstBattery();
    if ( not battery )
      return false;

    if ( battery->setChargeControlStartThreshold( value ) )
    {
      syslog( LOG_INFO, "Set charge start threshold to %d", value );
      return true;
    }
    
    syslog( LOG_WARNING, "Failed writing start threshold" );
    return false;
  }

  int getChargeEndThreshold() const noexcept
  {
    auto battery = PowerSupplyController::getFirstBattery();
    if ( not battery )
      return -1;

    return battery->getChargeControlEndThreshold();
  }

  bool setChargeEndThreshold( int value ) noexcept
  {
    auto battery = PowerSupplyController::getFirstBattery();
    if ( not battery )
      return false;

    if ( battery->setChargeControlEndThreshold( value ) )
    {
      syslog( LOG_INFO, "Set charge end threshold to %d", value );
      return true;
    }
    
    syslog( LOG_WARNING, "Failed writing end threshold" );
    return false;
  }

  // === Charge Type Methods ===

  std::string getChargeType() const noexcept
  {
    auto battery = PowerSupplyController::getFirstBattery();
    if ( not battery )
      return "Unknown";

    ChargeType type = battery->getChargeType();
    
    switch ( type )
    {
      case ChargeType::Trickle:      return "Trickle";
      case ChargeType::Fast:         return "Fast";
      case ChargeType::Standard:     return "Standard";
      case ChargeType::Adaptive:     return "Adaptive";
      case ChargeType::Custom:       return "Custom";
      case ChargeType::LongLife:     return "LongLife";
      case ChargeType::Bypass:       return "Bypass";
      case ChargeType::NotAvailable: return "N/A";
      default:                       return "Unknown";
    }
  }

  bool setChargeType( const std::string &type ) noexcept
  {
    auto battery = PowerSupplyController::getFirstBattery();
    if ( not battery )
      return false;

    if ( SysfsNode< std::string >( battery->getBasePath() + "/charge_type" ).write( type ) )
    {
      syslog( LOG_INFO, "Set charge type to %s", type.c_str() );
      return true;
    }
    
    syslog( LOG_WARNING, "Failed writing charge type" );
    return false;
  }

  // === JSON Serialization ===

  std::string getChargingProfilesAvailableJSON() const noexcept
  {
    auto profiles = getChargingProfilesAvailable();
    
    std::ostringstream oss;
    oss << "[";
    for ( size_t i = 0; i < profiles.size(); ++i )
    {
      if ( i > 0 )
        oss << ",";
      oss << "\"" << profiles[ i ] << "\"";
    }
    oss << "]";
    
    return oss.str();
  }

  std::string getChargingPrioritiesAvailableJSON() const noexcept
  {
    auto prios = getChargingPrioritiesAvailable();
    
    std::ostringstream oss;
    oss << "[";
    for ( size_t i = 0; i < prios.size(); ++i )
    {
      if ( i > 0 )
        oss << ",";
      oss << "\"" << prios[ i ] << "\"";
    }
    oss << "]";
    
    return oss.str();
  }

  std::string getChargeStartAvailableThresholdsJSON() const noexcept
  {
    auto thresholds = getChargeStartAvailableThresholds();
    
    std::ostringstream oss;
    oss << "[";
    for ( size_t i = 0; i < thresholds.size(); ++i )
    {
      if ( i > 0 )
        oss << ",";
      oss << thresholds[ i ];
    }
    oss << "]";
    
    return oss.str();
  }

  std::string getChargeEndAvailableThresholdsJSON() const noexcept
  {
    auto thresholds = getChargeEndAvailableThresholds();
    
    std::ostringstream oss;
    oss << "[";
    for ( size_t i = 0; i < thresholds.size(); ++i )
    {
      if ( i > 0 )
        oss << ",";
      oss << thresholds[ i ];
    }
    oss << "]";
    
    return oss.str();
  }

private:
  void initializeSettings() noexcept
  {
    // Initialize charging profile from hardware if available
    if ( hasChargingProfile() )
    {
      auto currentProfile = SysfsNode< std::string >( m_basePath + "/charging_profile" ).read().value_or( "" );
      if ( not currentProfile.empty() )
      {
        m_currentChargingProfile = currentProfile;
        syslog( LOG_INFO, "Initialized charging profile: %s", m_currentChargingProfile.c_str() );
      }
    }

    // Initialize charging priority from hardware if available
    if ( hasChargingPriority() )
    {
      auto currentPrio = SysfsNode< std::string >( "/sys/devices/platform/tuxedo_keyboard/charging_priority/charging_prio" ).read().value_or( "" );
      if ( not currentPrio.empty() )
      {
        m_currentChargingPriority = currentPrio;
        syslog( LOG_INFO, "Initialized charging priority: %s", m_currentChargingPriority.c_str() );
      }
    }
  }

  std::string m_basePath;
  std::string m_currentChargingProfile;
  std::string m_currentChargingPriority;
};
