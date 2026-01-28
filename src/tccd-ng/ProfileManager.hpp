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

#include "profiles/TccProfile.hpp"
#include "profiles/DefaultProfiles.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <optional>
#include <algorithm>
#include <cstdlib>
#include <sys/stat.h>
#include <cstring>
#include <iostream>

/**
 * @brief Manages TCC profile loading, saving, and manipulation
 * 
 * Handles reading/writing profiles from/to JSON files, profile validation,
 * and merging with default profiles. Mirrors TypeScript ConfigHandler functionality.
 */
class ProfileManager
{
public:
  /**
   * @brief Construct profile manager with default paths
   */
  ProfileManager()
    : m_profilesPath( "/etc/tcc/profiles" ),
      m_settingsPath( "/etc/tcc/settings" ),
      m_autosavePath( "/etc/tcc/autosave" ),
      m_fanTablesPath( "/etc/tcc/fantables" ),
      m_profileFileMod( 0644 ),
      m_settingsFileMod( 0644 )
  {
    std::memset( &m_lastProfileFileTime, 0, sizeof( m_lastProfileFileTime ) );
  }

  /**
   * @brief Construct with custom file paths
   */
  ProfileManager( const std::string &profilesPath,
                  const std::string &settingsPath,
                  const std::string &autosavePath,
                  const std::string &fanTablesPath )
    : m_profilesPath( profilesPath ),
      m_settingsPath( settingsPath ),
      m_autosavePath( autosavePath ),
      m_fanTablesPath( fanTablesPath ),
      m_profileFileMod( 0644 ),
      m_settingsFileMod( 0644 )
  {
    std::memset( &m_lastProfileFileTime, 0, sizeof( m_lastProfileFileTime ) );
  }

  /**
   * @brief Read custom profiles from disk
   * @return Vector of custom profiles, or empty vector on error
   */
  [[nodiscard]] std::vector< TccProfile > readProfiles() noexcept
  {
    try
    {
      // Update file timestamp before reading
      updateProfileFileTime();

      std::ifstream file( m_profilesPath );
      if ( !file.is_open() )
      {
        return {};
      }

      std::stringstream buffer;
      buffer << file.rdbuf();
      std::string content = buffer.str();

      auto profiles = parseProfilesJSON( content );
      
      // Get default profile template for filling missing fields
      TccProfile defaultTemplate = getDefaultCustomProfile();
      
      // Generate missing IDs and fill defaults (matches TypeScript implementation)
      bool modified = false;
      for ( auto &profile : profiles )
      {
        if ( profile.id.empty() )
        {
          profile.id = generateProfileId();
          modified = true;
        }
        
        // Fill missing fields from default template
        // This matches TypeScript's recursivelyFillObject behavior
        if ( fillMissingFields( profile, defaultTemplate ) )
        {
          modified = true;
        }
      }

      // Write back if modified (matches TypeScript behavior)
      if ( modified )
      {
        if ( writeProfiles( profiles ) )
        {
          std::cout << "[ProfileManager] Saved updated profiles with filled defaults" << std::endl;
        }
      }

      return profiles;
    }
    catch ( const std::exception &e )
    {
      return {};
    }
  }

  /**
   * @brief Write custom profiles to disk
   * @param profiles Vector of profiles to write
   * @return true on success, false on error
   */
  [[nodiscard]] bool writeProfiles( const std::vector< TccProfile > &profiles ) noexcept
  {
    try
    {
      // Create directory if it doesn't exist
      std::filesystem::path path( m_profilesPath );
      std::filesystem::path dir = path.parent_path();
      
      if ( !std::filesystem::exists( dir ) )
      {
        std::filesystem::create_directories( dir );
        std::filesystem::permissions( dir, 
          std::filesystem::perms::owner_all | 
          std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
          std::filesystem::perms::others_read | std::filesystem::perms::others_exec );
      }

      // Serialize to JSON
      std::string json = profilesToJSON( profiles );

      // Write to file
      std::ofstream file( m_profilesPath, std::ios::trunc );
      if ( !file.is_open() )
      {
        return false;
      }

      file << json;
      file.close();

      // Set file permissions
      chmod( m_profilesPath.c_str(), m_profileFileMod );

      // Update modification time tracker
      updateProfileFileTime();

      return true;
    }
    catch ( const std::exception &e )
    {
      return false;
    }
  }

  /**
   * @brief Get default custom profiles (template for new profiles)
   * @return Vector containing the default custom profile
   */
  [[nodiscard]] std::vector< TccProfile > getDefaultCustomProfiles() const noexcept
  {
    std::vector< TccProfile > profiles;
    profiles.push_back( getDefaultCustomProfile() );
    return profiles;
  }

  /**
   * @brief Get all default (hardcoded) profiles for a specific device
   * @param device Optional device identifier for device-specific profiles
   * @return Vector of default profiles
   */
  [[nodiscard]] std::vector< TccProfile > getDefaultProfiles( std::optional< TUXEDODevice > device = std::nullopt ) const noexcept
  {
    std::vector< TccProfile > result;
    
    // If device is specified, check for device-specific profiles
    if ( device.has_value() )
    {
      auto it = deviceProfiles.find( *device );
      if ( it != deviceProfiles.end() )
      {
        result = it->second;
        std::cout << "[ProfileManager::getDefaultProfiles] Returning " << result.size() << " device-specific profiles" << std::endl;
        return result;
      }
    }
    
    // Fallback to legacy profiles for unknown devices
    result = legacyProfiles;
    std::cout << "[ProfileManager::getDefaultProfiles] Returning " << result.size() << " legacy profiles" << std::endl;
    for ( size_t i = 0; i < result.size() && i < 3; ++i )
    {
      std::cout << "[ProfileManager::getDefaultProfiles]   Profile " << i << " (" << result[i].id 
                << ") has " << result[i].odmPowerLimits.tdpValues.size() << " TDP values";
      if ( !result[i].odmPowerLimits.tdpValues.empty() )
      {
        std::cout << ": [";
        for ( size_t j = 0; j < result[i].odmPowerLimits.tdpValues.size(); ++j )
        {
          if ( j > 0 ) std::cout << ", ";
          std::cout << result[i].odmPowerLimits.tdpValues[j];
        }
        std::cout << "]";
      }
      std::cout << std::endl;
    }
    return result;
  }

  /**
   * @brief Get custom profiles, returning defaults on error
   * @return Vector of custom profiles
   */
  [[nodiscard]] std::vector< TccProfile > getCustomProfilesNoThrow() noexcept
  {
    auto profiles = readProfiles();
    if ( profiles.empty() )
    {
      return getDefaultCustomProfiles();
    }
    return profiles;
  }

  /**
   * @brief Get all profiles (default + custom)
   * @return Vector containing all profiles
   */
  [[nodiscard]] std::vector< TccProfile > getAllProfiles() noexcept
  {
    auto defaultProfiles = getDefaultProfiles();
    auto customProfiles = getCustomProfilesNoThrow();
    
    defaultProfiles.insert( defaultProfiles.end(), 
                           customProfiles.begin(), 
                           customProfiles.end() );
    return defaultProfiles;
  }

  /**
   * @brief Find profile by ID
   * @param profiles Vector to search
   * @param id Profile ID to find
   * @return Pointer to profile if found, nullptr otherwise
   */
  [[nodiscard]] static const TccProfile *findProfileById( 
    const std::vector< TccProfile > &profiles, 
    const std::string &id ) noexcept
  {
    for ( const auto &profile : profiles )
    {
      if ( profile.id == id )
      {
        return &profile;
      }
    }
    return nullptr;
  }

  /**
   * @brief Find profile by name
   * @param profiles Vector to search
   * @param name Profile name to find
   * @return Pointer to profile if found, nullptr otherwise
   */
  [[nodiscard]] static const TccProfile *findProfileByName( 
    const std::vector< TccProfile > &profiles, 
    const std::string &name ) noexcept
  {
    for ( const auto &profile : profiles )
    {
      if ( profile.name == name )
      {
        return &profile;
      }
    }
    return nullptr;
  }

  /**
   * @brief Public wrapper to parse a fan table JSON array into entries
   * @param json JSON array string containing fan table entries
   * @return Vector of FanTableEntry parsed from JSON
   */
  [[nodiscard]] static std::vector< FanTableEntry > parseFanTableFromJSON( const std::string &json )
  {
    return parseFanTable( json );
  }

  /**
   * @brief Add a new custom profile
   * @param profile Profile to add
   * @return true on success, false on error
   */
  [[nodiscard]] bool addProfile( const TccProfile &profile ) noexcept
  {
    std::cout << "[ProfileManager] Adding profile '" << profile.name << "' (ID: " << profile.id << ")" << std::endl;
    auto profiles = getCustomProfilesNoThrow();
    std::cout << "[ProfileManager] Current profile count: " << profiles.size() << std::endl;
    profiles.push_back( profile );
    std::cout << "[ProfileManager] New profile count: " << profiles.size() << std::endl;
    
    bool result = writeProfiles( profiles );
    if ( result )
    {
      std::cout << "[ProfileManager] Successfully added profile to disk" << std::endl;
    }
    else
    {
      std::cerr << "[ProfileManager] Failed to write profiles to disk!" << std::endl;
    }
    return result;
  }

  /**
   * @brief Delete a custom profile by ID
   * @param profileId ID of profile to delete
   * @return true if deleted, false if not found or error
   */
  [[nodiscard]] bool deleteProfile( const std::string &profileId ) noexcept
  {
    auto profiles = getCustomProfilesNoThrow();
    auto it = std::remove_if( profiles.begin(), profiles.end(),
      [&profileId]( const TccProfile &p ) { return p.id == profileId; } );
    
    if ( it == profiles.end() )
    {
      return false; // Not found
    }

    profiles.erase( it, profiles.end() );
    return writeProfiles( profiles );
  }

  /**
   * @brief Update an existing custom profile
   * @param profile Updated profile (matched by ID)
   * @return true on success, false if not found or error
   */
  [[nodiscard]] bool updateProfile( const TccProfile &profile ) noexcept
  {
    auto profiles = getCustomProfilesNoThrow();
    bool found = false;
    
    std::cout << "[ProfileManager] Searching for profile ID '" << profile.id 
              << "' in " << profiles.size() << " profiles" << std::endl;
    
    for ( auto &p : profiles )
    {
      if ( p.id == profile.id )
      {
        std::cout << "[ProfileManager] Found profile '" << p.name << "', updating..." << std::endl;
        std::cout << "[ProfileManager]   Old fan control: " << (p.fan.useControl ? "enabled" : "disabled") << std::endl;
        std::cout << "[ProfileManager]   New fan control: " << (profile.fan.useControl ? "enabled" : "disabled") << std::endl;
        std::cout << "[ProfileManager]   Old fan profile: " << p.fan.fanProfile << std::endl;
        std::cout << "[ProfileManager]   New fan profile: " << profile.fan.fanProfile << std::endl;
        
        p = profile;
        found = true;
        break;
      }
    }

    if ( !found )
    {
      std::cerr << "[ProfileManager] Profile ID '" << profile.id << "' not found in custom profiles!" << std::endl;
      std::cerr << "[ProfileManager] Note: Default (hardcoded) profiles cannot be modified." << std::endl;
      std::cerr << "[ProfileManager] Available custom profile IDs:" << std::endl;
      for ( const auto &p : profiles )
      {
        std::cerr << "[ProfileManager]   - " << p.id << " (" << p.name << ")" << std::endl;
      }
      return false;
    }

    std::cout << "[ProfileManager] Writing updated profiles to disk..." << std::endl;
    bool result = writeProfiles( profiles );
    if ( result )
    {
      std::cout << "[ProfileManager] Successfully wrote profiles to disk" << std::endl;
    }
    else
    {
      std::cerr << "[ProfileManager] Failed to write profiles to disk!" << std::endl;
    }
    return result;
  }

  /**
   * @brief Copy a profile with a new ID and name
   * @param sourceId ID of profile to copy
   * @param newName Name for the new profile
   * @return Optional containing new profile on success
   */
  [[nodiscard]] std::optional< TccProfile > copyProfile( 
    const std::string &sourceId, 
    const std::string &newName ) noexcept
  {
    std::cout << "[ProfileManager] Attempting to copy profile ID: " << sourceId << std::endl;
    auto allProfiles = getAllProfiles();
    std::cout << "[ProfileManager] Total profiles available: " << allProfiles.size() << std::endl;
    
    const auto *source = findProfileById( allProfiles, sourceId );
    
    if ( !source )
    {
      std::cerr << "[ProfileManager] Source profile not found!" << std::endl;
      std::cerr << "[ProfileManager] Available profile IDs:" << std::endl;
      for ( const auto &p : allProfiles )
      {
        std::cerr << "[ProfileManager]   - " << p.id << " (" << p.name << ")" << std::endl;
      }
      return std::nullopt;
    }

    std::cout << "[ProfileManager] Found source profile: " << source->name << std::endl;
    TccProfile newProfile = *source;
    newProfile.id = generateProfileId();
    newProfile.name = newName;
    std::cout << "[ProfileManager] Generated new ID: " << newProfile.id << std::endl;

    if ( !addProfile( newProfile ) )
    {
      std::cerr << "[ProfileManager] Failed to add copied profile!" << std::endl;
      return std::nullopt;
    }

    std::cout << "[ProfileManager] Successfully copied profile" << std::endl;
    return newProfile;
  }

  const std::string &getProfilesPath() const { return m_profilesPath; }
  const std::string &getSettingsPath() const { return m_settingsPath; }

  /**
   * @brief Check if the profiles file has been modified externally since last read
   * @return true if file was modified, false otherwise
   */
  [[nodiscard]] bool profilesFileModified() noexcept
  {
    struct stat fileStat;
    if ( stat( m_profilesPath.c_str(), &fileStat ) != 0 )
    {
      // File doesn't exist or stat failed
      return false;
    }

    // Check if modification time has changed
    if ( fileStat.st_mtim.tv_sec != m_lastProfileFileTime.tv_sec ||
         fileStat.st_mtim.tv_nsec != m_lastProfileFileTime.tv_nsec )
    {
      // Update stored modification time
      m_lastProfileFileTime = fileStat.st_mtim;
      return true;
    }

    return false;
  }

  /**
   * @brief Force update of the file modification time without checking
   */
  void updateProfileFileTime() noexcept
  {
    struct stat fileStat;
    if ( stat( m_profilesPath.c_str(), &fileStat ) == 0 )
    {
      m_lastProfileFileTime = fileStat.st_mtim;
    }
  }

  /**
   * @brief Parse a single profile from JSON object
   * @param json JSON object string
   * @return Parsed profile
   */
  [[nodiscard]] static TccProfile parseProfileJSON( const std::string &json )
  {
    TccProfile profile;
    
    profile.id = extractString( json, "id" );
    profile.name = extractString( json, "name" );
    profile.description = extractString( json, "description" );

    // Parse display settings
    std::string displayJson = extractObject( json, "display" );
    if ( !displayJson.empty() )
    {
      profile.display.brightness = extractInt( displayJson, "brightness", 100 );
      profile.display.useBrightness = extractBool( displayJson, "useBrightness", false );
      profile.display.refreshRate = extractInt( displayJson, "refreshRate", -1 );
      profile.display.useRefRate = extractBool( displayJson, "useRefRate", false );
      profile.display.xResolution = extractInt( displayJson, "xResolution", -1 );
      profile.display.yResolution = extractInt( displayJson, "yResolution", -1 );
      profile.display.useResolution = extractBool( displayJson, "useResolution", false );
    }

    // Parse CPU settings
    std::string cpuJson = extractObject( json, "cpu" );
    if ( !cpuJson.empty() )
    {
      int32_t onlineCores = extractInt( cpuJson, "onlineCores", -1 );
      if ( onlineCores >= 0 )
      {
        profile.cpu.onlineCores = onlineCores;
      }
      
      profile.cpu.useMaxPerfGov = extractBool( cpuJson, "useMaxPerfGov", false );
      
      int32_t scalingMin = extractInt( cpuJson, "scalingMinFrequency", -1 );
      if ( scalingMin >= 0 )
      {
        profile.cpu.scalingMinFrequency = scalingMin;
      }
      
      int32_t scalingMax = extractInt( cpuJson, "scalingMaxFrequency", -1 );
      if ( scalingMax >= 0 )
      {
        profile.cpu.scalingMaxFrequency = scalingMax;
      }
      
      profile.cpu.governor = extractString( cpuJson, "governor", "powersave" );
      profile.cpu.energyPerformancePreference = extractString( cpuJson, "energyPerformancePreference", "balance_performance" );
      profile.cpu.noTurbo = extractBool( cpuJson, "noTurbo", false );
    }

    // Parse webcam settings
    std::string webcamJson = extractObject( json, "webcam" );
    if ( !webcamJson.empty() )
    {
      profile.webcam.status = extractBool( webcamJson, "status", true );
      profile.webcam.useStatus = extractBool( webcamJson, "useStatus", true );
    }

    // Parse fan settings
    std::string fanJson = extractObject( json, "fan" );
    if ( !fanJson.empty() )
    {
      profile.fan.useControl = extractBool( fanJson, "useControl", true );
      profile.fan.fanProfile = extractString( fanJson, "fanProfile", "Balanced" );
      profile.fan.minimumFanspeed = extractInt( fanJson, "minimumFanspeed", 0 );
      profile.fan.maximumFanspeed = extractInt( fanJson, "maximumFanspeed", 100 );
      profile.fan.offsetFanspeed = extractInt( fanJson, "offsetFanspeed", 0 );
      profile.fan.sameSpeed = extractBool( fanJson, "sameSpeed", true );
      
      // Debug: log the parsed fan offset and sameSpeed
      std::cout << "[ProfileManager] Parsed profile '" << profile.name 
                << "' offsetFanspeed: " << profile.fan.offsetFanspeed
                << " sameSpeed: " << ( profile.fan.sameSpeed ? "true" : "false" ) << std::endl;
      
      // No embedded custom fan tables allowed in profiles. Fan profiles should reference a named preset only.
      // Fan table data will be supplied by UCC via ApplyFanProfiles or persisted separately.
    }

    // Parse ODM profile
    std::string odmProfileJson = extractObject( json, "odmProfile" );
    if ( !odmProfileJson.empty() )
    {
      std::string odmName = extractString( odmProfileJson, "name" );
      if ( !odmName.empty() )
      {
        profile.odmProfile.name = odmName;
      }
    }

    // Parse ODM power limits
    std::string odmPowerJson = extractObject( json, "odmPowerLimits" );
    if ( !odmPowerJson.empty() )
    {
      profile.odmPowerLimits.tdpValues = extractIntArray( odmPowerJson, "tdpValues" );
    }

    // Parse NVIDIA power control
    std::string nvidiaJson = extractObject( json, "nvidiaPowerCTRLProfile" );
    if ( !nvidiaJson.empty() )
    {
      TccNVIDIAPowerCTRLProfile nvidiaProfile;
      nvidiaProfile.cTGPOffset = extractInt( nvidiaJson, "cTGPOffset", 0 );
      profile.nvidiaPowerCTRLProfile = nvidiaProfile;
    }

    return profile;
  }

  /**
   * @brief Get the default custom profile template
   */
  [[nodiscard]] static TccProfile getDefaultCustomProfile() noexcept
  {
    TccProfile profile;
    profile.id = "__default_custom_profile__";
    profile.name = "TUXEDO Defaults";
    profile.description = "Edit profile to change behaviour";
    
    // All fields already have sensible defaults from constructors
    return profile;
  }

  /**
   * @brief Parse JSON array of profiles
   * @param json JSON string containing profile array
   * @return Vector of parsed profiles
   */
  [[nodiscard]] static std::vector< TccProfile > parseProfilesJSON( const std::string &json )
  {
    std::vector< TccProfile > profiles;
    
    // Simple JSON array parser
    size_t pos = json.find( '[' );
    if ( pos == std::string::npos )
    {
      return profiles;
    }

    size_t depth = 0;
    size_t start = pos + 1;
    
    for ( size_t i = pos; i < json.length(); ++i )
    {
      char c = json[ i ];
      
      if ( c == '{' )
      {
        if ( depth == 0 )
        {
          start = i;
        }
        ++depth;
      }
      else if ( c == '}' )
      {
        --depth;
        if ( depth == 0 )
        {
          std::string profileJson = json.substr( start, i - start + 1 );
          auto profile = parseProfileJSON( profileJson );
          if ( !profile.id.empty() )
          {
            profiles.push_back( profile );
          }
        }
      }
    }

    return profiles;
  }

private:
  std::string m_profilesPath;
  std::string m_settingsPath;
  std::string m_autosavePath;
  std::string m_fanTablesPath;
  mode_t m_profileFileMod;
  mode_t m_settingsFileMod;
  struct timespec m_lastProfileFileTime;

  /**
   * @brief Fill missing fields in a profile from defaults (matches TypeScript recursivelyFillObject)
   * @param profile Profile to fill
   * @param defaultProfile Default profile template to fill from
   * @return true if any field was filled
   */
  [[nodiscard]] static bool fillMissingFields( TccProfile &profile, const TccProfile &defaultProfile ) noexcept
  {
    bool modified = false;
    
    // note: In C++ with our struct design, we use special sentinel values to indicate "undefined"
    // for optionals, we check has_value(). For ints, we use -1. For strings, empty string.

    // fill description if missing
    if ( profile.description.empty() and not defaultProfile.description.empty() )
    {
      profile.description = defaultProfile.description;
      modified = true;
    }

    // fill display fields
    // note: we don't fill these because they have valid defaults (brightness=100, refreshRate=-1, etc)
    // only fill if they were explicitly undefined, which in JSON would be null or missing
    
    // fill CPU fields
    if ( not profile.cpu.onlineCores.has_value() and defaultProfile.cpu.onlineCores.has_value() )
    {
      profile.cpu.onlineCores = defaultProfile.cpu.onlineCores;
      modified = true;
    }
    
    if ( not profile.cpu.scalingMinFrequency.has_value() and defaultProfile.cpu.scalingMinFrequency.has_value() )
    {
      profile.cpu.scalingMinFrequency = defaultProfile.cpu.scalingMinFrequency;
      modified = true;
    }
    
    if ( not profile.cpu.scalingMaxFrequency.has_value() and defaultProfile.cpu.scalingMaxFrequency.has_value() )
    {
      profile.cpu.scalingMaxFrequency = defaultProfile.cpu.scalingMaxFrequency;
      modified = true;
    }
    
    if ( profile.cpu.governor.empty() and not defaultProfile.cpu.governor.empty() )
    {
      profile.cpu.governor = defaultProfile.cpu.governor;
      modified = true;
    }
    
    if ( profile.cpu.energyPerformancePreference.empty() and not defaultProfile.cpu.energyPerformancePreference.empty() )
    {
      profile.cpu.energyPerformancePreference = defaultProfile.cpu.energyPerformancePreference;
      modified = true;
    }
    
    // fill fan profile if missing
    if ( profile.fan.fanProfile.empty() and not defaultProfile.fan.fanProfile.empty() )
    {
      profile.fan.fanProfile = defaultProfile.fan.fanProfile;
      modified = true;
    }

    // Profiles should not contain embedded fan tables. Fan presets are named references only.
    // Do not fill or copy any custom fan curve data here.
    
    // fill ODM profile name
    if ( not profile.odmProfile.name.has_value() and defaultProfile.odmProfile.name.has_value() )
    {
      profile.odmProfile.name = defaultProfile.odmProfile.name;
      modified = true;
    }
    
    return modified;
  }

  // Profiles are not allowed to embed full fan tables (customFanCurve). Fan tables are supplied
  // as named presets (via GetFanProfile) or sent at runtime via ApplyFanProfiles.

  /**
   * @brief Parse fan table from JSON array
   */
  [[nodiscard]] static std::vector< FanTableEntry > parseFanTable( const std::string &json )
  {
    std::vector< FanTableEntry > table;
    
    size_t depth = 0;
    size_t start = 0;
    
    for ( size_t i = 0; i < json.length(); ++i )
    {
      char c = json[ i ];
      
      if ( c == '{' )
      {
        if ( depth == 0 )
        {
          start = i;
        }
        ++depth;
      }
      else if ( c == '}' )
      {
        --depth;
        if ( depth == 0 )
        {
          std::string entryJson = json.substr( start, i - start + 1 );
          FanTableEntry entry;
          entry.temp = extractInt( entryJson, "temp", 0 );
          entry.speed = extractInt( entryJson, "speed", 0 );
          table.push_back( entry );
        }
      }
    }
    
    return table;
  }

  /**
   * @brief Serialize profiles to JSON array
   */
  [[nodiscard]] static std::string profilesToJSON( const std::vector< TccProfile > &profiles )
  {
    std::ostringstream oss;
    oss << "[";
    
    for ( size_t i = 0; i < profiles.size(); ++i )
    {
      if ( i > 0 )
      {
        oss << ",";
      }
      oss << profileToJSON( profiles[ i ] );
    }
    
    oss << "]";
    return oss.str();
  }

  /**
   * @brief Serialize single profile to JSON (complete format for file storage)
   */
  [[nodiscard]] static std::string profileToJSON( const TccProfile &profile )
  {
    std::ostringstream oss;
    
    oss << "{"
        << "\"id\":\"" << jsonEscape( profile.id ) << "\","
        << "\"name\":\"" << jsonEscape( profile.name ) << "\","
        << "\"description\":\"" << jsonEscape( profile.description ) << "\","
        << "\"display\":{"
        << "\"brightness\":" << profile.display.brightness << ","
        << "\"useBrightness\":" << ( profile.display.useBrightness ? "true" : "false" ) << ","
        << "\"refreshRate\":" << profile.display.refreshRate << ","
        << "\"useRefRate\":" << ( profile.display.useRefRate ? "true" : "false" ) << ","
        << "\"xResolution\":" << profile.display.xResolution << ","
        << "\"yResolution\":" << profile.display.yResolution << ","
        << "\"useResolution\":" << ( profile.display.useResolution ? "true" : "false" )
        << "},"
        << "\"cpu\":{"
        << "\"onlineCores\":" << ( profile.cpu.onlineCores.has_value() ? std::to_string( *profile.cpu.onlineCores ) : "-1" ) << ","
        << "\"useMaxPerfGov\":" << ( profile.cpu.useMaxPerfGov ? "true" : "false" ) << ","
        << "\"scalingMinFrequency\":" << ( profile.cpu.scalingMinFrequency.has_value() ? std::to_string( *profile.cpu.scalingMinFrequency ) : "-1" ) << ","
        << "\"scalingMaxFrequency\":" << ( profile.cpu.scalingMaxFrequency.has_value() ? std::to_string( *profile.cpu.scalingMaxFrequency ) : "-1" ) << ","
        << "\"governor\":\"" << jsonEscape( profile.cpu.governor ) << "\","
        << "\"energyPerformancePreference\":\"" << jsonEscape( profile.cpu.energyPerformancePreference ) << "\","
        << "\"noTurbo\":" << ( profile.cpu.noTurbo ? "true" : "false" )
        << "},"
        << "\"webcam\":{"
        << "\"status\":" << ( profile.webcam.status ? "true" : "false" ) << ","
        << "\"useStatus\":" << ( profile.webcam.useStatus ? "true" : "false" )
        << "},"
        << "\"fan\":{"
        << "\"useControl\":" << ( profile.fan.useControl ? "true" : "false" ) << ","
        << "\"fanProfile\":\"" << jsonEscape( profile.fan.fanProfile ) << "\","
        << "\"minimumFanspeed\":" << profile.fan.minimumFanspeed << ","
        << "\"maximumFanspeed\":" << profile.fan.maximumFanspeed << ","
        << "\"offsetFanspeed\":" << profile.fan.offsetFanspeed
        << "},"
        << "\"odmProfile\":{"
        << "\"name\":\"" << jsonEscape( profile.odmProfile.name.value_or( "" ) ) << "\""
        << "},"
        << "\"odmPowerLimits\":{"
        << "\"tdpValues\":[";
    
    for ( size_t i = 0; i < profile.odmPowerLimits.tdpValues.size(); ++i )
    {
      if ( i > 0 ) oss << ",";
      oss << profile.odmPowerLimits.tdpValues[ i ];
    }
    
    oss << "]},"
        << "\"nvidiaPowerCTRLProfile\":{"
        << "\"cTGPOffset\":" << ( profile.nvidiaPowerCTRLProfile.has_value() ? profile.nvidiaPowerCTRLProfile->cTGPOffset : 0 )
        << "}"
        << "}";
    
    return oss.str();
  }

  /**
   * @brief Serialize fan profile to JSON
   */


  /**
   * @brief Serialize fan table to JSON
   */
  [[nodiscard]] static std::string fanTableToJSON( const std::vector< FanTableEntry > &table )
  {
    std::ostringstream oss;
    oss << "[";
    
    for ( size_t i = 0; i < table.size(); ++i )
    {
      if ( i > 0 ) oss << ",";
      oss << "{"
          << "\"temp\":" << table[ i ].temp << ","
          << "\"speed\":" << table[ i ].speed
          << "}";
    }
    
    oss << "]";
    return oss.str();
  }

  // JSON parsing helper functions
  [[nodiscard]] static std::string extractString( const std::string &json, const std::string &key, const std::string &defaultValue = "" )
  {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find( searchKey );
    if ( pos == std::string::npos )
    {
      return defaultValue;
    }

    pos = json.find( ':', pos );
    if ( pos == std::string::npos )
    {
      return defaultValue;
    }

    pos = json.find( '"', pos );
    if ( pos == std::string::npos )
    {
      return defaultValue;
    }

    size_t end = json.find( '"', pos + 1 );
    if ( end == std::string::npos )
    {
      return defaultValue;
    }

    return json.substr( pos + 1, end - pos - 1 );
  }

  [[nodiscard]] static int32_t extractInt( const std::string &json, const std::string &key, int32_t defaultValue = 0 )
  {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find( searchKey );
    if ( pos == std::string::npos )
    {
      return defaultValue;
    }

    pos = json.find( ':', pos );
    if ( pos == std::string::npos )
    {
      return defaultValue;
    }

    // Skip whitespace
    ++pos;
    while ( pos < json.length() && std::isspace( json[ pos ] ) )
    {
      ++pos;
    }

    if ( pos >= json.length() )
    {
      return defaultValue;
    }

    // Parse number
    size_t end = pos;
    if ( json[ end ] == '-' )
    {
      ++end;
    }
    
    while ( end < json.length() && std::isdigit( json[ end ] ) )
    {
      ++end;
    }

    if ( end == pos || ( end == pos + 1 && json[ pos ] == '-' ) )
    {
      return defaultValue;
    }

    try
    {
      return std::stoi( json.substr( pos, end - pos ) );
    }
    catch ( ... )
    {
      return defaultValue;
    }
  }

  [[nodiscard]] static bool extractBool( const std::string &json, const std::string &key, bool defaultValue = false )
  {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find( searchKey );
    if ( pos == std::string::npos )
    {
      return defaultValue;
    }

    pos = json.find( ':', pos );
    if ( pos == std::string::npos )
    {
      return defaultValue;
    }

    size_t truePos = json.find( "true", pos );
    size_t falsePos = json.find( "false", pos );
    size_t commaPos = json.find( ',', pos );
    size_t bracePos = json.find( '}', pos );

    size_t nextDelimiter = std::min( commaPos, bracePos );

    if ( truePos != std::string::npos && truePos < nextDelimiter )
    {
      return true;
    }
    if ( falsePos != std::string::npos && falsePos < nextDelimiter )
    {
      return false;
    }

    return defaultValue;
  }

  [[nodiscard]] static std::string extractObject( const std::string &json, const std::string &key )
  {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find( searchKey );
    if ( pos == std::string::npos )
    {
      return "";
    }

    pos = json.find( ':', pos );
    if ( pos == std::string::npos )
    {
      return "";
    }

    pos = json.find( '{', pos );
    if ( pos == std::string::npos )
    {
      return "";
    }

    size_t depth = 1;
    size_t start = pos;
    ++pos;

    while ( pos < json.length() && depth > 0 )
    {
      if ( json[ pos ] == '{' )
      {
        ++depth;
      }
      else if ( json[ pos ] == '}' )
      {
        --depth;
      }
      ++pos;
    }

    if ( depth == 0 )
    {
      return json.substr( start, pos - start );
    }

    return "";
  }

  [[nodiscard]] static std::string extractArray( const std::string &json, const std::string &key )
  {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find( searchKey );
    if ( pos == std::string::npos )
    {
      return "";
    }

    pos = json.find( ':', pos );
    if ( pos == std::string::npos )
    {
      return "";
    }

    pos = json.find( '[', pos );
    if ( pos == std::string::npos )
    {
      return "";
    }

    size_t depth = 1;
    size_t start = pos;
    ++pos;

    while ( pos < json.length() && depth > 0 )
    {
      if ( json[ pos ] == '[' )
      {
        ++depth;
      }
      else if ( json[ pos ] == ']' )
      {
        --depth;
      }
      ++pos;
    }

    if ( depth == 0 )
    {
      return json.substr( start, pos - start );
    }

    return "";
  }

  [[nodiscard]] static std::vector< int32_t > extractIntArray( const std::string &json, const std::string &key )
  {
    std::vector< int32_t > result;
    std::string arrayJson = extractArray( json, key );
    
    if ( arrayJson.empty() )
    {
      return result;
    }

    size_t pos = 1; // Skip opening '['
    while ( pos < arrayJson.length() )
    {
      // Skip whitespace and commas
      while ( pos < arrayJson.length() && ( std::isspace( arrayJson[ pos ] ) || arrayJson[ pos ] == ',' ) )
      {
        ++pos;
      }

      if ( pos >= arrayJson.length() || arrayJson[ pos ] == ']' )
      {
        break;
      }

      // Parse number
      size_t start = pos;
      if ( arrayJson[ pos ] == '-' )
      {
        ++pos;
      }
      
      while ( pos < arrayJson.length() && std::isdigit( arrayJson[ pos ] ) )
      {
        ++pos;
      }

      if ( pos > start )
      {
        try
        {
          result.push_back( std::stoi( arrayJson.substr( start, pos - start ) ) );
        }
        catch ( ... )
        {
          // Skip invalid numbers
        }
      }
    }

    return result;
  }

  [[nodiscard]] static std::string jsonEscape( const std::string &value )
  {
    std::ostringstream oss;
    for ( const char c : value )
    {
      switch ( c )
      {
        case '"': oss << "\\\""; break;
        case '\\': oss << "\\\\"; break;
        case '\b': oss << "\\b"; break;
        case '\f': oss << "\\f"; break;
        case '\n': oss << "\\n"; break;
        case '\r': oss << "\\r"; break;
        case '\t': oss << "\\t"; break;
        default: oss << c; break;
      }
    }
    return oss.str();
  }
};
