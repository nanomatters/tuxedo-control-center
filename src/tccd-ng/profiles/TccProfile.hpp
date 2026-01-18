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

#include "FanProfile.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>

/**
 * @brief Display settings for a profile
 */
struct TccProfileDisplay
{
  int32_t brightness;
  bool useBrightness;
  int32_t refreshRate;
  bool useRefRate;
  int32_t xResolution;
  int32_t yResolution;
  bool useResolution;

  TccProfileDisplay()
    : brightness( 100 ),
      useBrightness( false ),
      refreshRate( -1 ),
      useRefRate( false ),
      xResolution( -1 ),
      yResolution( -1 ),
      useResolution( false )
  {
  }
};

/**
 * @brief CPU settings for a profile
 */
struct TccProfileCpu
{
  std::optional< int32_t > onlineCores;
  bool useMaxPerfGov;
  std::optional< int32_t > scalingMinFrequency;
  std::optional< int32_t > scalingMaxFrequency;
  std::string governor;
  std::string energyPerformancePreference;
  bool noTurbo;

  TccProfileCpu()
    : useMaxPerfGov( false ),
      governor( "powersave" ),
      energyPerformancePreference( "balance_performance" ),
      noTurbo( false )
  {
  }
};

/**
 * @brief Webcam settings for a profile
 */
struct TccProfileWebcam
{
  bool status;
  bool useStatus;

  TccProfileWebcam()
    : status( true ),
      useStatus( true )
  {
  }
};

/**
 * @brief Fan control settings for a profile
 */
struct TccProfileFanControl
{
  bool useControl;
  std::string fanProfile;
  int32_t minimumFanspeed;
  int32_t maximumFanspeed;
  int32_t offsetFanspeed;
  FanProfile customFanCurve;

  TccProfileFanControl()
    : useControl( true ),
      fanProfile( "Balanced" ),
      minimumFanspeed( 0 ),
      maximumFanspeed( 100 ),
      offsetFanspeed( 0 )
  {
  }
};

/**
 * @brief ODM profile settings
 */
struct TccODMProfile
{
  std::optional< std::string > name;

  TccODMProfile() = default;

  TccODMProfile( const std::string &profileName )
    : name( profileName )
  {
  }
};

/**
 * @brief ODM power limits
 */
struct TccODMPowerLimits
{
  std::vector< int32_t > tdpValues;

  TccODMPowerLimits() = default;

  TccODMPowerLimits( const std::vector< int32_t > &values )
    : tdpValues( values )
  {
  }
};

/**
 * @brief NVIDIA power control profile
 */
struct TccNVIDIAPowerCTRLProfile
{
  int32_t cTGPOffset;

  TccNVIDIAPowerCTRLProfile()
    : cTGPOffset( 0 )
  {
  }

  TccNVIDIAPowerCTRLProfile( int32_t offset )
    : cTGPOffset( offset )
  {
  }
};

/**
 * @brief Complete TCC profile
 *
 * Contains all settings for a system profile including CPU, display,
 * fan control, webcam, and ODM settings.
 */
struct TccProfile
{
  std::string id;
  std::string name;
  std::string description;
  TccProfileDisplay display;
  TccProfileCpu cpu;
  TccProfileWebcam webcam;
  TccProfileFanControl fan;
  TccODMProfile odmProfile;
  TccODMPowerLimits odmPowerLimits;
  std::optional< TccNVIDIAPowerCTRLProfile > nvidiaPowerCTRLProfile;

  TccProfile() = default;

  TccProfile( const std::string &profileId, const std::string &profileName )
    : id( profileId ),
      name( profileName )
  {
  }

  // deep copy constructor
  TccProfile( const TccProfile &other )
    : id( other.id ),
      name( other.name ),
      description( other.description ),
      display( other.display ),
      cpu( other.cpu ),
      webcam( other.webcam ),
      fan( other.fan ),
      odmProfile( other.odmProfile ),
      odmPowerLimits( other.odmPowerLimits ),
      nvidiaPowerCTRLProfile( other.nvidiaPowerCTRLProfile )
  {
  }

  // deep copy assignment
  TccProfile &operator=( const TccProfile &other )
  {
    if ( this != &other )
    {
      id = other.id;
      name = other.name;
      description = other.description;
      display = other.display;
      cpu = other.cpu;
      webcam = other.webcam;
      fan = other.fan;
      odmProfile = other.odmProfile;
      odmPowerLimits = other.odmPowerLimits;
      nvidiaPowerCTRLProfile = other.nvidiaPowerCTRLProfile;
    }
    return *this;
  }
};

/**
 * @brief Generate a unique profile ID
 * @return A unique profile identifier string
 */
std::string generateProfileId();

// legacy default profile IDs
namespace LegacyDefaultProfileIDs
{
  inline constexpr const char *Default = "__legacy_default__";
  inline constexpr const char *CoolAndBreezy = "__legacy_cool_and_breezy__";
  inline constexpr const char *PowersaveExtreme = "__legacy_powersave_extreme__";
}

// default profile ID constants
namespace DefaultProfileIDs
{
  inline constexpr const char *MaxEnergySave = "__profile_max_energy_save__";
  inline constexpr const char *Quiet = "__profile_silent__";
  inline constexpr const char *Office = "__office__";
  inline constexpr const char *HighPerformance = "__high_performance__";
}

// profile to image mapping
extern const std::map< std::string, std::string > profileImageMap;
