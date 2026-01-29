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
#include "../profiles/TccProfile.hpp"
#include "../SysfsNode.hpp"
#include <string>
#include <vector>
#include <functional>
#include <syslog.h>
#include <fstream>
#include <sstream>
#include <algorithm>

// Forward declaration
class TuxedoIOAPI;

/**
 * @brief ODMProfileWorker - Manages OEM/ODM performance profiles
 *
 * Applies vendor-specific performance profiles (quiet, balanced, performance, etc.)
 * using platform_profile sysfs interface or Tuxedo IO API.
 *
 * Detection priority:
 * 1. TUXEDO platform_profile (/sys/bus/platform/devices/tuxedo_platform_profile/)
 * 2. ACPI platform_profile (/sys/firmware/acpi/platform_profile)
 * 3. Tuxedo IO API (for legacy Clevo/Uniwill devices)
 */
class ODMProfileWorker : public DaemonWorker
{
public:
  ODMProfileWorker(
    TuxedoIOAPI *ioApi,
    std::function< TccProfile() > getActiveProfileCallback,
    std::function< void( const std::vector< std::string > & ) > setOdmProfilesAvailableCallback )
    : DaemonWorker( std::chrono::milliseconds( 10000 ), false ),
      m_ioApi( ioApi ),
      m_getActiveProfile( getActiveProfileCallback ),
      m_setOdmProfilesAvailable( setOdmProfilesAvailableCallback ),
      m_profileType( ProfileType::None )
  {
  }

  void onStart() override
  {
    detectProfileType();
    const TccProfile profile = m_getActiveProfile();
    if ( !profile.id.empty() )
    {
      applyODMProfile();
    }
  }

  void onWork() override
  {
    // Periodic work: currently nothing needed
    // ODM profile persists once set
  }

  void onExit() override
  {
    // Cleanup: currently nothing needed
  }

  /**
   * @brief Re-apply ODM profile from active profile
   * 
   * Call this when the profile changes
   */
  void reapplyProfile()
  {
    applyODMProfile();
  }

private:
  enum class ProfileType
  {
    None,
    TuxedoPlatformProfile,
    AcpiPlatformProfile,
    TuxedoIOAPI
  };

  TuxedoIOAPI *m_ioApi;
  std::function< TccProfile() > m_getActiveProfile;
  std::function< void( const std::vector< std::string > & ) > m_setOdmProfilesAvailable;
  ProfileType m_profileType;

  // Sysfs paths
  static constexpr const char *TUXEDO_PLATFORM_PROFILE = "/sys/bus/platform/devices/tuxedo_platform_profile/platform_profile";
  static constexpr const char *TUXEDO_PLATFORM_PROFILE_CHOICES = "/sys/bus/platform/devices/tuxedo_platform_profile/platform_profile_choices";
  static constexpr const char *ACPI_PLATFORM_PROFILE = "/sys/firmware/acpi/platform_profile";
  static constexpr const char *ACPI_PLATFORM_PROFILE_CHOICES = "/sys/firmware/acpi/platform_profile_choices";

  void detectProfileType()
  {
    // Priority 1: TUXEDO platform_profile
    if ( SysfsNode< std::string >( TUXEDO_PLATFORM_PROFILE ).isAvailable() and
         SysfsNode< std::string >( TUXEDO_PLATFORM_PROFILE_CHOICES ).isAvailable() )
    {
      m_profileType = ProfileType::TuxedoPlatformProfile;
      syslog( LOG_INFO, "ODMProfileWorker: Using TUXEDO platform_profile" );
      return;
    }

    // Priority 2: ACPI platform_profile
    if ( SysfsNode< std::string >( ACPI_PLATFORM_PROFILE ).isAvailable() and
         SysfsNode< std::string >( ACPI_PLATFORM_PROFILE_CHOICES ).isAvailable() )
    {
      m_profileType = ProfileType::AcpiPlatformProfile;
      syslog( LOG_INFO, "ODMProfileWorker: Using ACPI platform_profile" );
      return;
    }

    // Priority 3: Tuxedo IO API (legacy devices)
    if ( m_ioApi )
    {
      std::vector< std::string > availableProfiles;
      if ( getAvailableProfilesViaAPI( availableProfiles ) and not availableProfiles.empty() )
      {
        m_profileType = ProfileType::TuxedoIOAPI;
        syslog( LOG_INFO, "ODMProfileWorker: Using Tuxedo IO API" );
        return;
      }
    }

    m_profileType = ProfileType::None;
    syslog( LOG_INFO, "ODMProfileWorker: No ODM profile support available" );
  }

  std::vector< std::string > readPlatformProfileChoices( const char *path )
  {
    std::vector< std::string > profiles;
    
    std::ifstream file( path );
    if ( not file.is_open() )
      return profiles;

    std::string line;
    if ( std::getline( file, line ) )
    {
      // Parse space-separated list
      std::istringstream iss( line );
      std::string profile;
      while ( iss >> profile )
      {
        profiles.push_back( profile );
      }
    }

    return profiles;
  }

  bool getAvailableProfilesViaAPI( [[maybe_unused]] std::vector< std::string > &profiles )
  {
    if ( not m_ioApi )
      return false;

    // This would use TuxedoIOAPI::getAvailableODMPerformanceProfiles()
    // For now, return false as the API integration needs the actual implementation
    // The TypeScript version calls: ioAPI.getAvailableODMPerformanceProfiles(availableODMProfiles)
    return false;
  }

  std::string getDefaultProfileViaAPI()
  {
    if ( not m_ioApi )
      return "";

    // This would use TuxedoIOAPI::getDefaultODMPerformanceProfile()
    // The TypeScript version calls: ioAPI.getDefaultODMPerformanceProfile(defaultProfileName)
    return "";
  }

  bool setProfileViaAPI( [[maybe_unused]] const std::string &profileName )
  {
    if ( not m_ioApi )
      return false;

    // This would use TuxedoIOAPI::setODMPerformanceProfile()
    // The TypeScript version calls: ioAPI.setODMPerformanceProfile(chosenODMProfileName)
    return false;
  }

  void applyODMProfile()
  {
    const TccProfile profile = m_getActiveProfile();
    const std::string chosenProfileName = profile.odmProfile.name.value_or( "" );

    switch ( m_profileType )
    {
      case ProfileType::TuxedoPlatformProfile:
        applyPlatformProfile( TUXEDO_PLATFORM_PROFILE, TUXEDO_PLATFORM_PROFILE_CHOICES, chosenProfileName );
        break;

      case ProfileType::AcpiPlatformProfile:
        applyPlatformProfile( ACPI_PLATFORM_PROFILE, ACPI_PLATFORM_PROFILE_CHOICES, chosenProfileName );
        break;

      case ProfileType::TuxedoIOAPI:
        applyProfileViaAPI( chosenProfileName );
        break;

      case ProfileType::None:
        m_setOdmProfilesAvailable( {} );
        break;
    }
  }

  void applyPlatformProfile( const char *profilePath, const char *choicesPath, const std::string &chosenProfileName )
  {
    std::vector< std::string > availableProfiles = readPlatformProfileChoices( choicesPath );
    
    m_setOdmProfilesAvailable( availableProfiles );

    if ( chosenProfileName.empty() )
    {
      syslog( LOG_INFO, "ODMProfileWorker: No profile name specified in active profile" );
      return;
    }

    // Check if chosen profile is available
    auto it = std::find( availableProfiles.begin(), availableProfiles.end(), chosenProfileName );
    if ( it == availableProfiles.end() )
    {
      syslog( LOG_WARNING, "ODMProfileWorker: Profile '%s' not available", chosenProfileName.c_str() );
      return;
    }

    // Write profile
    SysfsNode< std::string > profileNode( profilePath );
    if ( profileNode.write( chosenProfileName ) )
    {
      syslog( LOG_INFO, "ODMProfileWorker: Set ODM profile to '%s'", chosenProfileName.c_str() );
    }
    else
    {
      syslog( LOG_WARNING, "ODMProfileWorker: Failed to set ODM profile to '%s'", chosenProfileName.c_str() );
    }
  }

  void applyProfileViaAPI( const std::string &chosenProfileName )
  {
    std::vector< std::string > availableProfiles;
    
    if ( not getAvailableProfilesViaAPI( availableProfiles ) )
    {
      syslog( LOG_WARNING, "ODMProfileWorker: Failed to get available profiles via API" );
      m_setOdmProfilesAvailable( {} );
      return;
    }

    m_setOdmProfilesAvailable( availableProfiles );

    std::string profileToApply = chosenProfileName;

    // If chosen profile not available, try default
    auto it = std::find( availableProfiles.begin(), availableProfiles.end(), profileToApply );
    if ( it == availableProfiles.end() )
    {
      profileToApply = getDefaultProfileViaAPI();
      syslog( LOG_INFO, "ODMProfileWorker: Profile '%s' not available, using default '%s'",
              chosenProfileName.c_str(), profileToApply.c_str() );
    }

    // Verify final profile is valid
    it = std::find( availableProfiles.begin(), availableProfiles.end(), profileToApply );
    if ( it == availableProfiles.end() )
    {
      syslog( LOG_WARNING, "ODMProfileWorker: No valid profile found" );
      return;
    }

    // Apply profile
    if ( setProfileViaAPI( profileToApply ) )
    {
      syslog( LOG_INFO, "ODMProfileWorker: Set ODM profile to '%s'", profileToApply.c_str() );
    }
    else
    {
      syslog( LOG_WARNING, "ODMProfileWorker: Failed to apply profile '%s'", profileToApply.c_str() );
    }
  }
};
