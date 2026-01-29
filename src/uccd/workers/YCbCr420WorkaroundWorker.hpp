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
#include "../TccSettings.hpp"
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

/**
 * @brief Worker for YCbCr 4:2:0 output workaround
 *
 * Enables forcing YUV420 output on specific DRI ports for HDMI 2.0 4K@60Hz compatibility.
 * Reads settings and applies force_yuv420_output sysfs attributes.
 */
class YCbCr420WorkaroundWorker : public DaemonWorker
{
public:
  YCbCr420WorkaroundWorker( TccSettings &settings, bool &modeReapplyPending )
    : DaemonWorker( std::chrono::milliseconds( 100000 ), false ),
      m_settings( settings ),
      m_modeReapplyPending( modeReapplyPending ),
      m_available( false )
  {
    checkAvailability();
  }

  bool isAvailable() const { return m_available; }

protected:
  void onStart() override
  {
    bool settings_changed = false;

    // Apply YCbCr420 workaround settings from config
    for ( const auto &cardEntry : m_settings.ycbcr420Workaround )
    {
      int card = cardEntry.card;
      for ( const auto &portEntry : cardEntry.ports )
      {
        std::string port = portEntry.port;
        bool enableYuv = portEntry.enabled;

        std::string path = "/sys/kernel/debug/dri/" + std::to_string( card ) + "/" + port + "/force_yuv420_output";

        if ( fileExists( path ) )
        {
          // Read current value
          std::ifstream file( path );
          if ( file.is_open() )
          {
            char currentValue;
            file.get( currentValue );
            file.close();

            bool oldValue = ( currentValue == '1' );
            if ( oldValue != enableYuv )
            {
              // Write new value
              std::ofstream outFile( path );
              if ( outFile.is_open() )
              {
                outFile << ( enableYuv ? "1" : "0" );
                outFile.close();
                settings_changed = true;
                std::cout << "[YCbCr420] Set " << path << " to " << ( enableYuv ? "1" : "0" ) << std::endl;
              }
              else
              {
                std::cerr << "[YCbCr420] Failed to write to " << path << std::endl;
              }
            }
          }
        }
      }
    }

    if ( settings_changed )
    {
      m_modeReapplyPending = true;
      std::cout << "[YCbCr420] Mode reapply pending due to YUV420 changes" << std::endl;
    }
  }

  void onWork() override
  {
    // No periodic work needed
  }

  void onExit() override
  {
    // No cleanup needed
  }

private:
  TccSettings &m_settings;
  bool &m_modeReapplyPending;
  bool m_available;

  bool fileExists( const std::string &path ) const
  {
    std::error_code ec;
    return fs::exists( path, ec ) && fs::is_regular_file( path, ec );
  }

  void checkAvailability()
  {
    m_available = false;

    if ( m_settings.ycbcr420Workaround.empty() )
    {
      return;
    }

    // Check if at least one configured path exists
    for ( const auto &cardEntry : m_settings.ycbcr420Workaround )
    {
      int card = cardEntry.card;
      for ( const auto &portEntry : cardEntry.ports )
      {
        std::string port = portEntry.port;
        std::string path = "/sys/kernel/debug/dri/" + std::to_string( card ) + "/" + port + "/force_yuv420_output";

        if ( fileExists( path ) )
        {
          m_available = true;
          return;
        }
      }
    }
  }
};
