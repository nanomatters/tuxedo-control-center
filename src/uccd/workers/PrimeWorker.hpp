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
#include <string>
#include <functional>
#include <filesystem>
#include <cstdio>
#include <syslog.h>
#include <thread>

/**
 * @brief PrimeWorker - NVIDIA Prime GPU switching support
 *
 * Manages NVIDIA Prime GPU switching on laptops with hybrid graphics (Intel/AMD + NVIDIA).
 * Monitors and reports the current Prime state.
 *
 * Prime States:
 * - "iGPU" - Power-saving mode using integrated GPU only
 * - "dGPU" - Performance mode using NVIDIA discrete GPU
 * - "on-demand" - Hybrid mode with GPU offloading
 * - "-1" - Prime not supported/available
 *
 * Requirements:
 * - prime-select utility (Ubuntu/TUXEDO OS)
 * - /var/lib/ubuntu-drivers-common/requires_offloading file
 *
 * Note: Only supports systems using prime-select (Ubuntu-based).
 * Other distributions may handle GPU switching differently.
 */
class PrimeWorker : public DaemonWorker
{
public:
  PrimeWorker( std::function< void( const std::string & ) > setPrimeStateCallback )
    : DaemonWorker( std::chrono::milliseconds( 10000 ), false ),
      m_setPrimeState( setPrimeStateCallback ),
      m_primeSupported( false )
  {
  }

  void onStart() override
  {
    // Wait for requires_offloading file to be updated after boot
    // (checking immediately may return wrong state)
    std::this_thread::sleep_for( std::chrono::milliseconds( 2000 ) );
    setPrimeStatus();
  }

  void onWork() override
  {
    // Check periodically in case someone changes state externally
    setPrimeStatus();
  }

  void onExit() override
  {
  }

  bool isPrimeSupported() const noexcept
  {
    return m_primeSupported;
  }

private:
  std::function< void( const std::string & ) > m_setPrimeState;
  bool m_primeSupported;

  void setPrimeStatus() noexcept
  {
    const bool primeSupported = checkPrimeSupported();

    if ( primeSupported )
    {
      m_setPrimeState( checkPrimeStatus() );
      m_primeSupported = primeSupported;
    }
    else
    {
      m_setPrimeState( "-1" );
      m_primeSupported = false;
    }
  }

  /**
   * @brief Check if Prime is supported on this system
   *
   * Requirements:
   * - /var/lib/ubuntu-drivers-common/requires_offloading file exists
   * - prime-select utility is available
   *
   * @return true if Prime is supported, false otherwise
   */
  bool checkPrimeSupported() const noexcept
  {
    // Check for requires_offloading file
    const bool offloadingStatus = std::filesystem::exists( "/var/lib/ubuntu-drivers-common/requires_offloading" );
    
    if ( not offloadingStatus )
      return false;

    // Check if prime-select is available
    try
    {
      auto pipe = popen( "which prime-select 2>/dev/null", "r" );
      if ( not pipe )
        return false;

      char buffer[256];
      std::string result;
      if ( fgets( buffer, sizeof( buffer ), pipe ) )
        result = buffer;
      
      pclose( pipe );

      // Trim whitespace
      while ( not result.empty() and ( result.back() == '\n' or result.back() == ' ' ) )
        result.pop_back();

      return not result.empty();
    }
    catch ( ... )
    {
      return false;
    }
  }

  /**
   * @brief Query current Prime status
   *
   * Executes: prime-select query
   *
   * @return Prime state string ("iGPU", "dGPU", "on-demand", or "off")
   */
  std::string checkPrimeStatus() const noexcept
  {
    try
    {
      auto pipe = popen( "prime-select query 2>/dev/null", "r" );
      if ( not pipe )
        return "off";

      char buffer[256];
      std::string result;
      if ( fgets( buffer, sizeof( buffer ), pipe ) )
        result = buffer;
      
      pclose( pipe );

      // Trim whitespace
      while ( not result.empty() and ( result.back() == '\n' or result.back() == ' ' or result.back() == '\r' ) )
        result.pop_back();

      return transformPrimeStatus( result );
    }
    catch ( ... )
    {
      return "off";
    }
  }

  /**
   * @brief Transform prime-select output to TCC format
   *
   * Converts:
   * - "nvidia" → "dGPU"
   * - "intel" → "iGPU"
   * - "on-demand" → "on-demand"
   * - others → "off"
   *
   * @param status Output from prime-select query
   * @return Transformed status string
   */
  std::string transformPrimeStatus( const std::string &status ) const noexcept
  {
    if ( status == "nvidia" )
      return "dGPU";
    else if ( status == "intel" )
      return "iGPU";
    else if ( status == "on-demand" )
      return "on-demand";
    else
      return "off";
  }
};
