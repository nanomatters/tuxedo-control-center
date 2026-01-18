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
#include <functional>
#include <vector>
#include <string>
#include <sstream>
#include <syslog.h>

// forward declaration
class TuxedoIOAPI;

/**
 * @brief TDP information structure
 */
struct TDPInfo
{
  uint32_t min;
  uint32_t max;
  uint32_t current;
  std::string descriptor;
};

/**
 * @brief Worker for managing ODM power limits (TDP)
 *
 * Applies TDP (Thermal Design Power) values from the active profile
 * using the Tuxedo IO API.
 */
class ODMPowerLimitWorker : public DaemonWorker
{
public:
  explicit ODMPowerLimitWorker(
    TuxedoIOAPI *ioApi,
    std::function< TccProfile() > getActiveProfile,
    std::function< void( const std::string & ) > setOdmPowerLimitsJSON,
    std::function< void( const std::string & ) > logFunction )
    : DaemonWorker( std::chrono::milliseconds( 5000 ), false )
    , m_ioApi( ioApi )
    , m_getActiveProfile( std::move( getActiveProfile ) )
    , m_setOdmPowerLimitsJSON( std::move( setOdmPowerLimitsJSON ) )
    , m_logFunction( std::move( logFunction ) )
  {}

  void onStart() override
  {
    logLine( "ODMPowerLimitWorker: onStart() called" );
    applyODMPowerLimits();
  }

  void onWork() override
  {
    // periodic work: currently nothing needed
    // TDP values persist once set
  }

  void onExit() override
  {
    // cleanup: currently nothing needed
  }

  /**
   * @brief Re-apply TDP from active profile
   * 
   * Call this when the profile changes to re-apply TDP values
   */
  void reapplyProfile()
  {
    logLine( "ODMPowerLimitWorker: reapplyProfile() called" );
    applyODMPowerLimits();
  }

  /**
   * @brief Get TDP information from hardware
   */
  std::vector< TDPInfo > getTDPInfo()
  {
    std::vector< TDPInfo > tdpInfo;

    if ( not m_ioApi )
      return tdpInfo;

    int nrTDPs = 0;

    if ( not m_ioApi->getNumberTDPs( nrTDPs ) or nrTDPs <= 0 )
      return tdpInfo;

    std::vector< std::string > descriptors;
    m_ioApi->getTDPDescriptors( descriptors );

    for ( int i = 0; i < nrTDPs; ++i )
    {
      TDPInfo info;
      info.current = 0;
      info.min = 0;
      info.max = 0;
      info.descriptor = ( i < static_cast< int >( descriptors.size() ) ) ? descriptors[ i ] : "";

      m_ioApi->getTDPMin( i, reinterpret_cast< int & >( info.min ) );
      m_ioApi->getTDPMax( i, reinterpret_cast< int & >( info.max ) );
      m_ioApi->getTDP( i, reinterpret_cast< int & >( info.current ) );

      tdpInfo.push_back( info );
    }

    return tdpInfo;
  }

  /**
   * @brief Set TDP values
   */
  bool setTDPValues( const std::vector< uint32_t > &values )
  {
    if ( not m_ioApi )
      return false;

    bool allSuccess = true;

    for ( size_t i = 0; i < values.size(); ++i )
    {
      if ( not m_ioApi->setTDP( static_cast< int >( i ), static_cast< int >( values[ i ] ) ) )
      {
        allSuccess = false;
      }
    }

    return allSuccess;
  }

private:
  TuxedoIOAPI *m_ioApi;
  std::function< TccProfile() > m_getActiveProfile;
  std::function< void( const std::string & ) > m_setOdmPowerLimitsJSON;
  std::function< void( const std::string & ) > m_logFunction;

  void logLine( const std::string &message )
  {
    if ( m_logFunction )
    {
      m_logFunction( message );
    }

    syslog( LOG_INFO, "%s", message.c_str() );
  }

  /**
   * @brief Apply ODM power limits from active profile
   */
  void applyODMPowerLimits()
  {
    logLine( "ODMPowerLimitWorker: applyODMPowerLimits() called" );
    
    const TccProfile profile = m_getActiveProfile();
    const auto &odmPowerLimits = profile.odmPowerLimits;

    auto tdpInfo = getTDPInfo();

    if ( tdpInfo.empty() )
    {
      logLine( "ODMPowerLimitWorker: No TDP hardware available" );
      // no TDP hardware available
      m_setOdmPowerLimitsJSON( "[]" );
      return;
    }
    
    logLine( "ODMPowerLimitWorker: Found " + std::to_string( tdpInfo.size() ) + " TDP descriptors" );

    std::vector< uint32_t > newTDPValues;

    // use profile values if available
    if ( not odmPowerLimits.tdpValues.empty() )
    {
      for ( int val : odmPowerLimits.tdpValues )
        newTDPValues.push_back( static_cast< uint32_t >( val ) );
    }

    // default to max values if not set
    if ( newTDPValues.empty() )
    {
      for ( const auto &tdp : tdpInfo )
      {
        newTDPValues.push_back( tdp.max );
      }
    }

    // build log message
    std::ostringstream logMessage;
    logMessage << "ODMPowerLimitWorker: Set ODM TDPs [";

    for ( size_t i = 0; i < newTDPValues.size(); ++i )
    {
      if ( i > 0 )
        logMessage << ", ";

      logMessage << newTDPValues[ i ] << " W";
    }

    logMessage << "]";
    logLine( logMessage.str() );

    // write TDP values
    const bool writeSuccess = setTDPValues( newTDPValues );

    if ( writeSuccess )
    {
      // update current values in tdpInfo
      for ( size_t i = 0; i < tdpInfo.size() and i < newTDPValues.size(); ++i )
      {
        tdpInfo[ i ].current = newTDPValues[ i ];
      }
    }
    else
    {
      logLine( "ODMPowerLimitWorker: Failed to write TDP values" );
    }

    // convert tdpInfo to JSON and update DBus data
    std::ostringstream jsonStream;
    jsonStream << "[";

    for ( size_t i = 0; i < tdpInfo.size(); ++i )
    {
      if ( i > 0 )
        jsonStream << ",";

      jsonStream << "{"
                 << "\"current\":" << tdpInfo[ i ].current << ","
                 << "\"min\":" << tdpInfo[ i ].min << ","
                 << "\"max\":" << tdpInfo[ i ].max
                 << "}";
    }

    jsonStream << "]";

    m_setOdmPowerLimitsJSON( jsonStream.str() );
  }
};
