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

#include "FanProfile.hpp"

// helper function to generate full temperature table (0-100)
static std::vector< FanTableEntry > generateTable(
  const std::vector< std::pair< int32_t, int32_t > > &keyPoints )
{
  std::vector< FanTableEntry > table;
  table.reserve( 101 );

  for ( int32_t temp = 0; temp <= 100; ++temp )
  {
    int32_t speed = 0;

    // find the appropriate range
    for ( size_t i = 0; i < keyPoints.size(); ++i )
    {
      if ( temp <= keyPoints[ i ].first )
      {
        speed = keyPoints[ i ].second;
        break;
      }

      if ( i < keyPoints.size() - 1 )
      {
        if ( temp > keyPoints[ i ].first and temp <= keyPoints[ i + 1 ].first )
        {
          // interpolate between keyPoints[i] and keyPoints[i+1]
          int32_t t1 = keyPoints[ i ].first;
          int32_t t2 = keyPoints[ i + 1 ].first;
          int32_t s1 = keyPoints[ i ].second;
          int32_t s2 = keyPoints[ i + 1 ].second;

          if ( t2 - t1 > 0 )
            speed = s1 + ( s2 - s1 ) * ( temp - t1 ) / ( t2 - t1 );
          else
            speed = s1;

          break;
        }
      }
      else
      {
        speed = keyPoints.back().second;
      }
    }

    table.push_back( FanTableEntry( temp, speed ) );
  }

  return table;
}

const std::vector< FanProfile > defaultFanProfiles = {
  // Silent profile
  FanProfile(
    "Silent",
    generateTable( {
      { 60, 0 }, { 65, 20 }, { 69, 25 }, { 71, 30 }, { 73, 35 },
      { 75, 40 }, { 77, 45 }, { 79, 50 }, { 81, 55 }, { 83, 60 },
      { 85, 65 }, { 87, 70 }, { 88, 75 }, { 89, 80 }, { 90, 85 },
      { 92, 90 }, { 94, 95 }, { 100, 100 }
    } ),
    generateTable( {
      { 59, 0 }, { 61, 20 }, { 63, 22 }, { 65, 24 }, { 67, 25 },
      { 69, 30 }, { 71, 37 }, { 73, 43 }, { 75, 46 }, { 77, 52 },
      { 81, 60 }, { 83, 65 }, { 85, 70 }, { 87, 80 }, { 89, 90 },
      { 100, 100 }
    } )
  ),

  // Quiet profile
  FanProfile(
    "Quiet",
    generateTable( {
      { 50, 0 }, { 60, 20 }, { 64, 23 }, { 66, 25 }, { 68, 28 },
      { 70, 33 }, { 72, 40 }, { 74, 44 }, { 76, 48 }, { 78, 52 },
      { 80, 55 }, { 82, 60 }, { 84, 65 }, { 86, 70 }, { 88, 80 },
      { 90, 85 }, { 92, 90 }, { 94, 95 }, { 100, 100 }
    } ),
    generateTable( {
      { 50, 0 }, { 60, 20 }, { 64, 25 }, { 68, 30 }, { 72, 40 },
      { 74, 44 }, { 76, 48 }, { 78, 52 }, { 80, 55 }, { 82, 60 },
      { 84, 65 }, { 86, 70 }, { 88, 80 }, { 90, 90 }, { 100, 100 }
    } )
  ),

  // Balanced profile
  FanProfile(
    "Balanced",
    generateTable( {
      { 45, 0 }, { 51, 20 }, { 53, 23 }, { 56, 26 }, { 59, 30 },
      { 62, 33 }, { 64, 35 }, { 66, 40 }, { 68, 45 }, { 70, 50 },
      { 72, 52 }, { 74, 53 }, { 76, 57 }, { 78, 60 }, { 80, 65 },
      { 82, 70 }, { 84, 75 }, { 86, 80 }, { 88, 85 }, { 91, 90 },
      { 94, 95 }, { 100, 100 }
    } ),
    generateTable( {
      { 45, 0 }, { 51, 20 }, { 53, 23 }, { 56, 26 }, { 59, 30 },
      { 62, 33 }, { 64, 35 }, { 66, 40 }, { 68, 45 }, { 70, 50 },
      { 72, 52 }, { 74, 53 }, { 76, 57 }, { 78, 60 }, { 80, 65 },
      { 82, 70 }, { 84, 75 }, { 86, 80 }, { 88, 85 }, { 90, 90 },
      { 100, 100 }
    } )
  ),

  // Cool profile
  FanProfile(
    "Cool",
    generateTable( {
      { 39, 0 }, { 45, 20 }, { 50, 25 }, { 56, 30 }, { 60, 35 },
      { 64, 42 }, { 67, 45 }, { 70, 50 }, { 73, 55 }, { 76, 60 },
      { 79, 70 }, { 82, 75 }, { 85, 85 }, { 89, 90 }, { 93, 95 },
      { 100, 100 }
    } ),
    generateTable( {
      { 39, 0 }, { 44, 25 }, { 49, 30 }, { 54, 35 }, { 59, 40 },
      { 64, 45 }, { 69, 50 }, { 74, 60 }, { 79, 70 }, { 84, 75 },
      { 88, 90 }, { 91, 100 }
    } )
  ),

  // Freezy profile
  FanProfile(
    "Freezy",
    generateTable( {
      { 29, 20 }, { 39, 25 }, { 45, 30 }, { 50, 40 }, { 55, 40 },
      { 60, 45 }, { 65, 50 }, { 70, 55 }, { 75, 60 }, { 77, 65 },
      { 79, 70 }, { 82, 80 }, { 85, 85 }, { 89, 90 }, { 94, 95 },
      { 100, 100 }
    } ),
    generateTable( {
      { 35, 25 }, { 40, 30 }, { 45, 35 }, { 50, 40 }, { 55, 45 },
      { 60, 50 }, { 65, 60 }, { 70, 65 }, { 75, 70 }, { 80, 75 },
      { 85, 85 }, { 89, 95 }, { 100, 100 }
    } )
  ),

  // Custom profile (empty tables, to be filled by user)
  FanProfile( "Custom" )
};

const FanProfile customFanPreset(
  "CustomPreset",
  {
    FanTableEntry( 20, 12 ),
    FanTableEntry( 30, 14 ),
    FanTableEntry( 40, 22 ),
    FanTableEntry( 50, 35 ),
    FanTableEntry( 60, 44 ),
    FanTableEntry( 70, 56 ),
    FanTableEntry( 80, 79 ),
    FanTableEntry( 90, 85 ),
    FanTableEntry( 100, 90 )
  },
  {
    FanTableEntry( 20, 12 ),
    FanTableEntry( 30, 14 ),
    FanTableEntry( 40, 22 ),
    FanTableEntry( 50, 35 ),
    FanTableEntry( 60, 44 ),
    FanTableEntry( 70, 56 ),
    FanTableEntry( 80, 79 ),
    FanTableEntry( 90, 85 ),
    FanTableEntry( 100, 90 )
  }
);
