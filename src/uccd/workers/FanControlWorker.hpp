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
#include "../profiles/FanProfile.hpp"
#include "../../native-lib/tuxedo_io_lib/tuxedo_io_api.hh"
#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <functional>
#include <syslog.h>

enum class FanLogicType { CPU, GPU };

class ValueBuffer
{
public:
  ValueBuffer() : m_bufferMaxSize( 13 ) {}

  void addValue( int value )
  {
    m_bufferData.push_back( value );

    if ( m_bufferData.size() > m_bufferMaxSize )
      m_bufferData.erase( m_bufferData.begin() );
  }

  int getFilteredValue( void ) const
  {
    if ( m_bufferData.empty() )
      return 0;

    // number of values to use for averaging (7 middle values)
    const size_t usedSize = 7;

    std::vector< int > copy = m_bufferData;
    std::sort( copy.begin(), copy.end() );

    // remove outliers from both ends
    while ( copy.size() >= usedSize + 2 )
    {
      copy.erase( copy.begin() );
      copy.pop_back();
    }

    // calculate average
    int sum = std::accumulate( copy.begin(), copy.end(), 0 );
    return static_cast< int >( std::round( static_cast< double >( sum ) / static_cast< double >( copy.size() ) ) );
  }

private:
  std::vector< int > m_bufferData;
  size_t m_bufferMaxSize;
};

class FanControlLogic
{
public:
  FanControlLogic( const FanProfile &fanProfile, FanLogicType type )
    : m_fanProfile( fanProfile )
    , m_type( type )
    , m_latestSpeedPercent( 0 )
    , m_lastSpeed( 0 )
    , m_fansMinSpeedHWLimit( 0 )
    , m_fansOffAvailable( true )
    , m_minimumFanspeed( 0 )
    , m_maximumFanspeed( 100 )
    , m_offsetFanspeed( 0 )
  {
    updateFanProfile( fanProfile );
  }

  void setFansMinSpeedHWLimit( int speed )
  { m_fansMinSpeedHWLimit = std::clamp( speed, 0, 100 ); }

  void setFansOffAvailable( bool available )
  { m_fansOffAvailable = available; }

  void setMinimumFanspeed( int speed )
  { m_minimumFanspeed = std::clamp( speed, 0, 100 ); }

  void setMaximumFanspeed( int speed )
  { m_maximumFanspeed = std::clamp( speed, 0, 100 ); }

  void setOffsetFanspeed( int speed )
  { m_offsetFanspeed = std::clamp( speed, -100, 100 ); }

  void updateFanProfile( const FanProfile &fanProfile )
  {
    m_fanProfile = fanProfile;
    
    const auto &table = getTable();
    if ( !table.empty() )
    {
      m_tableMinEntry = table.front();
      m_tableMaxEntry = table.back();
    }
  }

  void reportTemperature( int temperatureValue )
  {
    m_tempBuffer.addValue( temperatureValue );
    m_latestSpeedPercent = calculateSpeedPercent();
  }

  int getSpeedPercent() const
  { return m_latestSpeedPercent; }

  const FanProfile &getFanProfile() const
  { return m_fanProfile; }

private:
  const std::vector< FanTableEntry > &getTable() const
  {
    return m_type == FanLogicType::CPU ? m_fanProfile.tableCPU : m_fanProfile.tableGPU;
  }

  int applyHwFanLimitations( int speed ) const
  {
    const int minSpeed = m_fansMinSpeedHWLimit;
    const int halfMinSpeed = minSpeed / 2;

    if ( speed < minSpeed )
    {
      if ( m_fansOffAvailable && speed < halfMinSpeed )
      {
        return 0;
      }
      else if ( m_fansOffAvailable || speed >= halfMinSpeed )
      {
        return minSpeed;
      }
    }

    return speed;
  }

  int limitFanSpeedChange( int speed )
  {
    constexpr int MAX_SPEED_JUMP = 2;
    constexpr int SPEED_JUMP_THRESHOLD = 20;

    const int speedJump = speed - m_lastSpeed;
    const bool isJumpTooBig =
      m_lastSpeed > SPEED_JUMP_THRESHOLD &&
      speedJump <= -MAX_SPEED_JUMP;

    return isJumpTooBig ? m_lastSpeed - MAX_SPEED_JUMP : speed;
  }

  size_t findFittingEntryIndex( int temperatureValue ) const
  {
    const auto &table = getTable();
    
    if ( table.empty() )
      return 0;

    if ( temperatureValue >= m_tableMaxEntry.temp )
    {
      return table.size() - 1;
    }
    else if ( temperatureValue <= m_tableMinEntry.temp )
    {
      return 0;
    }

    // Find exact match or next higher temperature
    for ( size_t i = 0; i < table.size(); ++i )
    {
      if ( table[i].temp >= temperatureValue )
      {
        return i;
      }
    }

    return table.size() - 1;
  }

  int manageCriticalTemperature( int temp, int speed ) const
  {
    // From common/classes/FanUtils.ts
    constexpr int CRITICAL_TEMPERATURE = 85;
    constexpr int OVERHEAT_TEMPERATURE = 90;

    if ( temp >= OVERHEAT_TEMPERATURE )
    {
      return 100;
    }
    else if ( temp >= CRITICAL_TEMPERATURE )
    {
      return std::max( speed, 80 );
    }

    return speed;
  }

  int calculateSpeedPercent()
  {
    const int temp = m_tempBuffer.getFilteredValue();
    const size_t entryIndex = findFittingEntryIndex( temp );
    
    const auto &table = getTable();
    if ( table.empty() )
      return 0;

    int speed = table[entryIndex].speed;

    // Apply offset
    speed += m_offsetFanspeed;

    // Apply min/max limits
    speed = std::max( m_minimumFanspeed, std::min( m_maximumFanspeed, speed ) );
    speed = std::clamp( speed, 0, 100 );

    // Apply hardware limitations
    speed = applyHwFanLimitations( speed );
    
    // Limit speed change rate
    speed = limitFanSpeedChange( speed );
    
    // Handle critical temperatures
    speed = manageCriticalTemperature( temp, speed );

    m_lastSpeed = speed;
    return speed;
  }

  FanProfile m_fanProfile;
  FanLogicType m_type;
  ValueBuffer m_tempBuffer;
  int m_latestSpeedPercent;
  int m_lastSpeed;
  
  FanTableEntry m_tableMinEntry;
  FanTableEntry m_tableMaxEntry;

  int m_fansMinSpeedHWLimit;
  bool m_fansOffAvailable;
  int m_minimumFanspeed;
  int m_maximumFanspeed;
  int m_offsetFanspeed;
};

class FanControlWorker : public DaemonWorker
{
public:
  FanControlWorker(
    TuxedoIOAPI *io,
    std::function< UccProfile() > getActiveProfile,
    std::function< bool() > getFanControlEnabled,
    std::function< void( size_t, int64_t, int ) > updateFanSpeed,
    std::function< void( size_t, int64_t, int ) > updateFanTemp
  )
    : DaemonWorker( std::chrono::milliseconds( 1000 ) )
    , m_io( io )
    , m_getActiveProfile( getActiveProfile )
    , m_getFanControlEnabled( getFanControlEnabled )
    , m_updateFanSpeed( updateFanSpeed )
    , m_updateFanTemp( updateFanTemp )
    , m_modeSameSpeed( true )
    , m_controlAvailableMessageShown( false )
    , m_hasTemporaryCurves( false )
  {
  }

  ~FanControlWorker() override = default;

  // Allow external callers to change same-speed mode at runtime
  void setSameSpeed( bool same )
  {
    m_modeSameSpeed = same;
    syslog( LOG_INFO, "FanControlWorker: setSameSpeed = %d", m_modeSameSpeed ? 1 : 0 );
  }

  [[nodiscard]] bool getSameSpeed() const noexcept { return m_modeSameSpeed; }

  /**
   * @brief Clear temporary fan curves and revert to profile curves
   */
  void clearTemporaryCurves()
  {
    m_hasTemporaryCurves = false;
    m_tempCpuTable.clear();
    m_tempGpuTable.clear();
  }
  void applyTemporaryFanCurves( const std::vector< FanTableEntry > &cpuTable,
                                const std::vector< FanTableEntry > &gpuTable )
  {
    // Store the temporary curves
    m_tempCpuTable = cpuTable;
    m_tempGpuTable = gpuTable;
    m_hasTemporaryCurves = true;
    
    for ( size_t i = 0; i < m_fanLogics.size(); ++i )
    {
      FanProfile tempProfile = m_fanLogics[i].getFanProfile(); // Copy current profile
      
      if ( i == 0 && !cpuTable.empty() ) // CPU fan (index 0)
      {
        tempProfile.tableCPU = cpuTable;
      }
      else if ( i > 0 && !gpuTable.empty() ) // GPU fans (index > 0)
      {
        tempProfile.tableGPU = gpuTable;
      }
      
      m_fanLogics[i].updateFanProfile( tempProfile );
    }
  }

protected:
  void onStart() override
  {
    int numberFans = 0;
    bool fansDetected = m_io->getNumberFans( numberFans ) && numberFans > 0;
    
    // If getNumberFans fails, try to detect fans by reading temperature from fan 0
    if ( !fansDetected )
    {
      int temp = -1;
      if ( m_io->getFanTemperature( 0, temp ) && temp >= 0 )
      {
        // We can read from at least fan 0, assume we have CPU and GPU fans
        numberFans = 2;
        fansDetected = true;
        syslog( LOG_INFO, "FanControlWorker: Detected fans by temperature reading (getNumberFans failed)" );
      }
    }
    
    if ( fansDetected && numberFans > 0 )
    {
      // Initialize fan logic for each fan
      for ( int i = 0; i < numberFans; ++i )
      {
        auto profile = m_getActiveProfile();
        
        // Fan 0 is CPU, others are GPU
        FanLogicType type = ( i == 0 ) ? FanLogicType::CPU : FanLogicType::GPU;
        // Resolve the named fan profile preset
        FanProfile fp = getDefaultFanProfileByName( profile.fan.fanProfile );
        m_fanLogics.emplace_back( fp, type );
      }

      // Get hardware fan limits
      int minSpeed = 0;
      if ( m_io->getFansMinSpeed( minSpeed ) )
      {
        m_fansMinSpeedHWLimit = minSpeed;
      }

      bool fansOffAvailable = true;
      if ( m_io->getFansOffAvailable( fansOffAvailable ) )
      {
        m_fansOffAvailable = fansOffAvailable;
      }

      // Apply hardware limits to all fan logics
      for ( auto &logic : m_fanLogics )
      {
        logic.setFansMinSpeedHWLimit( m_fansMinSpeedHWLimit );
        logic.setFansOffAvailable( m_fansOffAvailable );
      }

      syslog( LOG_INFO, "FanControlWorker started with %d fans", numberFans );
    }
    else
    {
      syslog( LOG_INFO, "FanControlWorker: No fans detected" );
    }
  }

  void onWork() override
  {
    if ( m_fanLogics.empty() )
    {
      if ( !m_controlAvailableMessageShown )
      {
        syslog( LOG_INFO, "FanControlWorker: Control unavailable (no fans)" );
        m_controlAvailableMessageShown = true;
      }
      return;
    }

    if ( m_controlAvailableMessageShown )
    {
      syslog( LOG_INFO, "FanControlWorker: Control resumed" );
      m_controlAvailableMessageShown = false;
    }

    // Get current profile and update fan logics if profile changed
    auto profile = m_getActiveProfile();
    if ( !profile.id.empty() )
    {
      updateFanLogicsFromProfile( profile );
    }

    const bool useFanControl = m_getFanControlEnabled();
    const int64_t timestamp = std::chrono::duration_cast< std::chrono::milliseconds >(
      std::chrono::system_clock::now().time_since_epoch() ).count();

    std::vector< int > fanTemps;
    std::vector< int > fanSpeedsSet;
    std::vector< bool > tempSensorAvailable;

    // Read temperatures and calculate fan speeds
    for ( size_t fanIndex = 0; fanIndex < m_fanLogics.size(); ++fanIndex )
    {
      int tempCelsius = -1;
      bool tempReadSuccess = m_io->getFanTemperature( static_cast< int >( fanIndex ), tempCelsius );

      tempSensorAvailable.push_back( tempReadSuccess );

      if ( tempReadSuccess )
      {
        fanTemps.push_back( tempCelsius );
        
        // Report temperature to logic and get calculated speed
        m_fanLogics[fanIndex].reportTemperature( tempCelsius );
        int calculatedSpeed = m_fanLogics[fanIndex].getSpeedPercent();
        fanSpeedsSet.push_back( calculatedSpeed );
      }
      else
      {
        fanTemps.push_back( -1 );
        fanSpeedsSet.push_back( 0 );
      }
    }

    // Write fan speeds if fan control is enabled
    if ( useFanControl )
    {
      // Find highest speed for "same speed" mode
      int highestSpeed = 0;
      for ( int speed : fanSpeedsSet )
      {
        if ( speed > highestSpeed )
          highestSpeed = speed;
      }

      // Set fan speeds
      for ( size_t fanIndex = 0; fanIndex < m_fanLogics.size(); ++fanIndex )
      {
        int speedToSet = fanSpeedsSet[fanIndex];
        
        // Use highest speed if in "same speed" mode or no sensor available
        if ( m_modeSameSpeed || !tempSensorAvailable[fanIndex] )
        {
          speedToSet = highestSpeed;
          fanSpeedsSet[fanIndex] = highestSpeed;
        }

        // Log the temperature and speed we are about to set for debugging
        syslog( LOG_INFO, "FanControlWorker: fan %d temp=%d calculated=%d set=%d sameSpeed=%d",
                static_cast< int >( fanIndex ), fanTemps[fanIndex], fanSpeedsSet[fanIndex], speedToSet, m_modeSameSpeed ? 1 : 0 );

        m_io->setFanSpeedPercent( static_cast< int >( fanIndex ), speedToSet );
      }
    }

    // Publish data via callbacks
    for ( size_t fanIndex = 0; fanIndex < m_fanLogics.size(); ++fanIndex )
    {
      int currentSpeed;

      if ( fanTemps[fanIndex] == -1 )
      {
        currentSpeed = -1;
      }
      else if ( useFanControl )
      {
        // Report calculated/set speed when control is active
        currentSpeed = fanSpeedsSet[fanIndex];
      }
      else
      {
        // Report hardware speed when control is disabled
        int hwSpeed = -1;
        m_io->getFanSpeedPercent( static_cast< int >( fanIndex ), hwSpeed );
        currentSpeed = hwSpeed;
      }

      m_updateFanTemp( fanIndex, timestamp, fanTemps[fanIndex] );
      m_updateFanSpeed( fanIndex, timestamp, currentSpeed );
    }
  }

  void onExit() override
  {
    // Nothing special to do on exit
  }

private:
  void updateFanLogicsFromProfile( const UccProfile &profile )
  {
    // Respect profile setting for same-speed mode
    m_modeSameSpeed = profile.fan.sameSpeed;
    syslog( LOG_INFO, "FanControlWorker: sameSpeed mode = %d", m_modeSameSpeed ? 1 : 0 );

    for ( size_t i = 0; i < m_fanLogics.size(); ++i )
    {
      // Resolve fan profile by name (built-in) instead of using embedded tables in profiles
      FanProfile fanProfile;
      for ( const auto &p : defaultFanProfiles )
      {
        if ( p.name == profile.fan.fanProfile ) { fanProfile = p; break; }
      }
      // Fallbacks
      if ( !fanProfile.isValid() )
      {
        for ( const auto &p : defaultFanProfiles ) { if ( p.name == "Balanced" ) { fanProfile = p; break; } }
      }
      if ( !fanProfile.isValid() && !defaultFanProfiles.empty() ) fanProfile = defaultFanProfiles[0];
      
      // If temporary curves are active, use them instead of profile curves
      if ( m_hasTemporaryCurves )
      {
        if ( i == 0 && !m_tempCpuTable.empty() ) // CPU fan (index 0)
        {
          fanProfile.tableCPU = m_tempCpuTable;
        }
        else if ( i > 0 && !m_tempGpuTable.empty() ) // GPU fans (index > 0)
        {
          fanProfile.tableGPU = m_tempGpuTable;
        }
      }
      
      m_fanLogics[i].updateFanProfile( fanProfile );
      m_fanLogics[i].setMinimumFanspeed( profile.fan.minimumFanspeed );
      m_fanLogics[i].setMaximumFanspeed( profile.fan.maximumFanspeed );
      m_fanLogics[i].setOffsetFanspeed( profile.fan.offsetFanspeed );
    }
  }

  TuxedoIOAPI *m_io;
  std::function< UccProfile() > m_getActiveProfile;
  std::function< bool() > m_getFanControlEnabled;
  std::function< void( size_t, int64_t, int ) > m_updateFanSpeed;
  std::function< void( size_t, int64_t, int ) > m_updateFanTemp;

  std::vector< FanControlLogic > m_fanLogics;
  bool m_modeSameSpeed;
  bool m_controlAvailableMessageShown;
  int m_fansMinSpeedHWLimit;
  bool m_fansOffAvailable;
  
  // Temporary fan curve tracking
  bool m_hasTemporaryCurves;
  std::vector< FanTableEntry > m_tempCpuTable;
  std::vector< FanTableEntry > m_tempGpuTable;
};
