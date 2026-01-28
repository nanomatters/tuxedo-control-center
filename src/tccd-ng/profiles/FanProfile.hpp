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

#include <string>
#include <vector>
#include <cstdint>
#include <cmath>

/**
 * @brief Fan table entry
 *
 * Represents a single temperature/speed mapping point in a fan curve.
 */
struct FanTableEntry
{
  int32_t temp;   // temperature in degrees Celsius
  int32_t speed;  // fan speed in percentage (0-100)

  FanTableEntry() : temp( 0 ), speed( 0 ) {}

  FanTableEntry( int32_t t, int32_t s ) : temp( t ), speed( s ) {}
};

/**
 * @brief Fan profile
 *
 * Contains temperature-to-fan-speed mapping tables for CPU and GPU fans.
 * Mirrors the ITccFanProfile TypeScript interface.
 */
class FanProfile
{
public:
  std::string name;
  std::vector< FanTableEntry > tableCPU;
  std::vector< FanTableEntry > tableGPU;

  FanProfile() = default;

  FanProfile( const std::string &profileName )
    : name( profileName )
  {
  }

  FanProfile( const std::string &profileName,
              const std::vector< FanTableEntry > &cpu,
              const std::vector< FanTableEntry > &gpu )
    : name( profileName ),
      tableCPU( cpu ),
      tableGPU( gpu )
  {
  }

  /**
   * @brief Check if profile has valid data
   * @return true if both CPU and GPU tables are populated
   */
  [[nodiscard]] bool isValid() const noexcept
  {
    return not tableCPU.empty() and not tableGPU.empty();
  }

  /**
   * @brief Get fan speed for a given temperature
   *
   * @param temp Temperature in degrees Celsius
   * @param useCPU true to use CPU table, false for GPU table
   * @return Fan speed percentage (0-100), or -1 if not found
   */
  [[nodiscard]] int32_t getSpeedForTemp( int32_t temp, bool useCPU = true ) const noexcept
  {
    const auto &table = useCPU ? tableCPU : tableGPU;

    if ( table.empty() )
      return -1;

    // If temp is at or below first entry, return first speed
    if ( temp <= table.front().temp )
      return table.front().speed;

    // find exact match or interpolate
    for ( size_t i = 1; i < table.size(); ++i )
    {
      const auto &prev = table[i-1];
      const auto &entry = table[i];

      if ( entry.temp == temp ) return entry.speed;

      if ( temp > prev.temp && temp < entry.temp )
      {
        int32_t tempDiff = entry.temp - prev.temp;
        if ( tempDiff == 0 ) return prev.speed;
        double frac = static_cast<double>( temp - prev.temp ) / static_cast<double>( tempDiff );
        return static_cast<int32_t>( std::lround( prev.speed + frac * ( entry.speed - prev.speed ) ) );
      }
    }

    // temperature is beyond the table, return last speed
    return table.back().speed;
  }
};

// default fan profile presets
extern const std::vector< FanProfile > defaultFanProfiles;
extern const FanProfile customFanPreset;

std::string getFanProfileJson(const std::string &name);
bool setFanProfileJson(const std::string &name, const std::string &json);

// Return a FanProfile by name from the built-in presets. Falls back to Balanced or first profile.
FanProfile getDefaultFanProfileByName( const std::string &name );
