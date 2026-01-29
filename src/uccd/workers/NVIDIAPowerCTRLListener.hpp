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
#include "../profiles/UccProfile.hpp"
#include "../SysfsNode.hpp"
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <array>
#include <memory>
#include <cstdio>
#include <functional>

namespace fs = std::filesystem;

/**
 * @brief Listener for NVIDIA power control
 *
 * Manages NVIDIA GPU power limits through the tuxedo_nvidia_power_ctrl kernel module.
 * Monitors and applies cTGP (configurable Total Graphics Power) offset from profiles.
 */
class NVIDIAPowerCTRLListener : public DaemonWorker
{
public:
  NVIDIAPowerCTRLListener(
    std::function< UccProfile() > getActiveProfile,
    int32_t &defaultPowerLimit,
    int32_t &maxPowerLimit,
    bool &available
  )
    : DaemonWorker( std::chrono::milliseconds( 5000 ), false ),
      m_getActiveProfile( getActiveProfile ),
      m_ctgpOffsetPath( "/sys/devices/platform/tuxedo_nvidia_power_ctrl/ctgp_offset" ),
      m_lastAppliedOffset( 0 ),
      m_defaultPowerLimit( defaultPowerLimit ),
      m_maxPowerLimit( maxPowerLimit ),
      m_available( available )
  {
    m_available = checkAvailability();
    
    if ( m_available )
    {
      queryPowerLimits();
    }
  }

  bool isAvailable() const { return m_available; }

  void onActiveProfileChanged()
  {
    if ( !m_available )
      return;

    applyActiveProfile();
  }

protected:
  void onStart() override
  {
    if ( !m_available )
      return;

    applyActiveProfile();
  }

  void onWork() override
  {
    if ( !m_available )
      return;

    // Check if external changes occurred and re-apply if needed
    std::ifstream file( m_ctgpOffsetPath );
    if ( file.is_open() )
    {
      int32_t currentValue = 0;
      file >> currentValue;
      file.close();

      int32_t expectedOffset = getProfileOffset();

      if ( currentValue != expectedOffset )
      {
        std::cout << "[NVIDIAPowerCTRL] External change detected (current: " << currentValue 
                  << ", expected: " << expectedOffset << "), re-applying profile" << std::endl;
        applyActiveProfile();
      }
    }
  }

  void onExit() override
  {
    // No cleanup needed
  }

private:
  std::function< UccProfile() > m_getActiveProfile;
  std::string m_ctgpOffsetPath;
  int32_t m_lastAppliedOffset;
  int32_t &m_defaultPowerLimit;
  int32_t &m_maxPowerLimit;
  bool &m_available;

  bool checkAvailability() const
  {
    std::error_code ec;
    return fs::exists( m_ctgpOffsetPath, ec ) && fs::is_regular_file( m_ctgpOffsetPath, ec );
  }

  int32_t getProfileOffset() const
  {
    UccProfile profile = m_getActiveProfile();
    
    if ( profile.nvidiaPowerCTRLProfile.has_value() )
    {
      return profile.nvidiaPowerCTRLProfile->cTGPOffset;
    }
    
    return 0; // Default offset
  }

  void applyActiveProfile()
  {
    int32_t ctgpOffset = getProfileOffset();

    std::ofstream file( m_ctgpOffsetPath );
    if ( !file.is_open() )
    {
      std::cerr << "[NVIDIAPowerCTRL] Failed to open " << m_ctgpOffsetPath << " for writing" << std::endl;
      return;
    }

    file << ctgpOffset;
    file.flush();

    // Check if write was successful
    if ( !file.good() )
    {
      std::cerr << "[NVIDIAPowerCTRL] Failed to write cTGP offset to " << m_ctgpOffsetPath 
                << " (stream error)" << std::endl;
      return;
    }

    file.close();

    // Verify the write by reading back
    std::ifstream verifyFile( m_ctgpOffsetPath );
    if ( verifyFile.is_open() )
    {
      int32_t verifiedValue = -1;
      verifyFile >> verifiedValue;
      verifyFile.close();

      if ( verifiedValue == ctgpOffset )
      {
        m_lastAppliedOffset = ctgpOffset;
        std::cout << "[NVIDIAPowerCTRL] Applied cTGP offset: " << ctgpOffset << std::endl;
      }
      else
      {
        std::cerr << "[NVIDIAPowerCTRL] Write verification failed - wrote " << ctgpOffset 
                  << " but read back " << verifiedValue << std::endl;
      }
    }
  }

  void queryPowerLimits()
  {
    // Query default power limit
    m_defaultPowerLimit = executeNvidiaSmi( "nvidia-smi --format=csv,noheader,nounits --query-gpu=power.default_limit" );
    
    // Query max power limit
    m_maxPowerLimit = executeNvidiaSmi( "nvidia-smi --format=csv,noheader,nounits --query-gpu=power.max_limit" );

    std::cout << "[NVIDIAPowerCTRL] NVIDIA GPU power limits - Default: " << m_defaultPowerLimit 
              << "W, Max: " << m_maxPowerLimit << "W" << std::endl;
  }

  int32_t executeNvidiaSmi( const std::string &command ) const
  {
    std::array< char, 128 > buffer;
    std::string result;

    auto pipeDeleter = []( FILE *fp ) { if ( fp ) pclose( fp ); };
    std::unique_ptr< FILE, decltype( pipeDeleter ) > pipe( popen( command.c_str(), "r" ), pipeDeleter );
    if ( !pipe )
    {
      std::cerr << "[NVIDIAPowerCTRL] Failed to execute: " << command << std::endl;
      return 0;
    }

    while ( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr )
    {
      result += buffer.data();
    }

    // Trim whitespace and convert to int
    result.erase( 0, result.find_first_not_of( " \t\n\r" ) );
    result.erase( result.find_last_not_of( " \t\n\r" ) + 1 );

    try
    {
      return std::stoi( result );
    }
    catch ( ... )
    {
      std::cerr << "[NVIDIAPowerCTRL] Failed to parse nvidia-smi output: " << result << std::endl;
      return 0;
    }
  }
};
