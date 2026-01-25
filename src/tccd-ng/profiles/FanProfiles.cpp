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
#include <cmath>
#include <sstream>
#include <string>

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
            speed = s1 + static_cast< int32_t >( std::lround( static_cast< double >( s2 - s1 ) * ( temp - t1 ) / ( t2 - t1 ) ) );
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
      { 20, 0 }, { 25, 0 }, { 30, 0 }, { 35, 0 }, { 40, 0 }, { 45, 0 },
      { 50, 0 }, { 55, 0 }, { 60, 0 }, { 65, 20 }, { 70, 28 }, { 75, 40 },
      { 80, 53 }, { 85, 65 }, { 90, 83 }, { 95, 96 }, { 100, 100 }
    } ),
    generateTable( {
      { 20, 0 }, { 25, 0 }, { 30, 0 }, { 35, 0 }, { 40, 0 }, { 45, 0 },
      { 50, 0 }, { 55, 0 }, { 60, 10 }, { 65, 24 }, { 70, 34 }, { 75, 46 },
      { 80, 58 }, { 85, 70 }, { 90, 91 }, { 95, 95 }, { 100, 100 }
    } )
  ),

  // Quiet profile
  FanProfile(
    "Quiet",
    generateTable( {
      { 20, 0 }, { 25, 0 }, { 30, 0 }, { 35, 0 }, { 40, 0 }, { 45, 0 },
      { 50, 0 }, { 55, 10 }, { 60, 20 }, { 65, 24 }, { 70, 33 }, { 75, 46 },
      { 80, 55 }, { 85, 68 }, { 90, 85 }, { 95, 96 }, { 100, 100 }
    } ),
    generateTable( {
      { 20, 0 }, { 25, 0 }, { 30, 0 }, { 35, 0 }, { 40, 0 }, { 45, 0 },
      { 50, 0 }, { 55, 10 }, { 60, 20 }, { 65, 26 }, { 70, 35 }, { 75, 46 },
      { 80, 55 }, { 85, 68 }, { 90, 90 }, { 95, 95 }, { 100, 100 }
    } )
  ),

  // Balanced profile
  FanProfile(
    "Balanced",
    generateTable( {
      { 20, 0 }, { 25, 0 }, { 30, 0 }, { 35, 0 }, { 40, 0 }, { 45, 0 },
      { 50, 17 }, { 55, 25 }, { 60, 31 }, { 65, 38 }, { 70, 50 }, { 75, 55 },
      { 80, 65 }, { 85, 78 }, { 90, 88 }, { 95, 96 }, { 100, 100 }
    } ),
    generateTable( {
      { 20, 0 }, { 25, 0 }, { 30, 0 }, { 35, 0 }, { 40, 0 }, { 45, 0 },
      { 50, 17 }, { 55, 25 }, { 60, 31 }, { 65, 38 }, { 70, 50 }, { 75, 55 },
      { 80, 65 }, { 85, 78 }, { 90, 90 }, { 95, 95 }, { 100, 100 }
    } )
  ),

  // Cool profile
  FanProfile(
    "Cool",
    generateTable( {
      { 20, 0 }, { 25, 0 }, { 30, 0 }, { 35, 0 }, { 40, 3 }, { 45, 20 },
      { 50, 25 }, { 55, 29 }, { 60, 35 }, { 65, 43 }, { 70, 50 }, { 75, 58 },
      { 80, 72 }, { 85, 85 }, { 90, 93 }, { 95, 96 }, { 100, 100 }
    } ),
    generateTable( {
      { 20, 0 }, { 25, 0 }, { 30, 0 }, { 35, 0 }, { 40, 5 }, { 45, 26 },
      { 50, 31 }, { 55, 36 }, { 60, 41 }, { 65, 46 }, { 70, 52 }, { 75, 62 },
      { 80, 71 }, { 85, 79 }, { 90, 97 }, { 95, 100 }, { 100, 100 }
    } )
  ),

  // Freezy profile
  FanProfile(
    "Freezy",
    generateTable( {
      { 20, 20 }, { 25, 20 }, { 30, 21 }, { 35, 23 }, { 40, 26 }, { 45, 30 },
      { 50, 40 }, { 55, 40 }, { 60, 45 }, { 65, 50 }, { 70, 55 }, { 75, 60 },
      { 80, 73 }, { 85, 85 }, { 90, 91 }, { 95, 96 }, { 100, 100 }
    } ),
    generateTable( {
      { 20, 25 }, { 25, 25 }, { 30, 25 }, { 35, 25 }, { 40, 30 }, { 45, 35 },
      { 50, 40 }, { 55, 45 }, { 60, 50 }, { 65, 60 }, { 70, 65 }, { 75, 70 },
      { 80, 75 }, { 85, 85 }, { 90, 95 }, { 95, 98 }, { 100, 100 }
    } )
  )
};

static FanProfile customFanProfile("Custom");

std::string getFanProfileJson(const std::string &name)
{
  const FanProfile *fp = nullptr;
  if (name == "Custom") {
    fp = &customFanProfile;
  } else {
    for (const auto &p : defaultFanProfiles) {
      if (p.name == name) {
        fp = &p;
        break;
      }
    }
  }
  if (!fp) return "{}";

  std::string json = "{";
  
  // tableCPU - return every 6th point for GUI editing (reduce from 101 to ~17 points)
  json += "\"tableCPU\":[";
  for (size_t i = 20; i < fp->tableCPU.size(); i += 6) {  // Start from 20°C, step by 6
    const auto &entry = fp->tableCPU[i];
    json += "{\"temp\":" + std::to_string(entry.temp) + ",\"speed\":" + std::to_string(entry.speed) + "}";
    if (i + 6 < fp->tableCPU.size()) json += ",";
  }
  json += "],";
  
  // tableGPU - return every 6th point for GUI editing
  json += "\"tableGPU\":[";
  for (size_t i = 20; i < fp->tableGPU.size(); i += 6) {  // Start from 20°C, step by 6
    const auto &entry = fp->tableGPU[i];
    json += "{\"temp\":" + std::to_string(entry.temp) + ",\"speed\":" + std::to_string(entry.speed) + "}";
    if (i + 6 < fp->tableGPU.size()) json += ",";
  }
  json += "]";
  
  json += "}";
  return json;
}

bool setFanProfileJson(const std::string &name, const std::string &json)
{
  if (name != "Custom") return false;

  // Parse the key points from JSON
  std::vector<std::pair<int32_t, int32_t>> cpuKeyPoints;
  std::vector<std::pair<int32_t, int32_t>> gpuKeyPoints;

  // Parse tableCPU
  size_t start = json.find("\"tableCPU\":[");
  if (start == std::string::npos) return false;
  start += 12; // length of "tableCPU":[
  size_t end = json.find("]", start);
  if (end == std::string::npos) return false;
  std::string cpuStr = json.substr(start, end - start);

  size_t pos = 0;
  while (pos < cpuStr.size()) {
    size_t objStart = cpuStr.find('{', pos);
    if (objStart == std::string::npos) break;
    size_t objEnd = cpuStr.find('}', objStart);
    if (objEnd == std::string::npos) break;
    std::string obj = cpuStr.substr(objStart, objEnd - objStart + 1);

    size_t tempPos = obj.find("\"temp\":");
    if (tempPos == std::string::npos) continue;
    tempPos += 7;
    size_t tempEnd = obj.find(',', tempPos);
    if (tempEnd == std::string::npos) tempEnd = obj.find('}', tempPos);
    int temp = std::stoi(obj.substr(tempPos, tempEnd - tempPos));

    size_t speedPos = obj.find("\"speed\":");
    if (speedPos == std::string::npos) continue;
    speedPos += 8;
    size_t speedEnd = obj.find('}', speedPos);
    int speed = std::stoi(obj.substr(speedPos, speedEnd - speedPos));

    cpuKeyPoints.emplace_back(temp, speed);
    pos = objEnd + 1;
  }

  // Parse tableGPU
  start = json.find("\"tableGPU\":[");
  if (start == std::string::npos) return false;
  start += 12;
  end = json.find("]", start);
  if (end == std::string::npos) return false;
  std::string gpuStr = json.substr(start, end - start);

  pos = 0;
  while (pos < gpuStr.size()) {
    size_t objStart = gpuStr.find('{', pos);
    if (objStart == std::string::npos) break;
    size_t objEnd = gpuStr.find('}', objStart);
    if (objEnd == std::string::npos) break;
    std::string obj = gpuStr.substr(objStart, objEnd - objStart + 1);

    size_t tempPos = obj.find("\"temp\":");
    if (tempPos == std::string::npos) continue;
    tempPos += 7;
    size_t tempEnd = obj.find(',', tempPos);
    if (tempEnd == std::string::npos) tempEnd = obj.find('}', tempPos);
    int temp = std::stoi(obj.substr(tempPos, tempEnd - tempPos));

    size_t speedPos = obj.find("\"speed\":");
    if (speedPos == std::string::npos) continue;
    speedPos += 8;
    size_t speedEnd = obj.find('}', speedPos);
    int speed = std::stoi(obj.substr(speedPos, speedEnd - speedPos));

    gpuKeyPoints.emplace_back(temp, speed);
    pos = objEnd + 1;
  }

  // Generate full tables from key points
  std::vector<FanTableEntry> cpuTable = generateTable(cpuKeyPoints);
  std::vector<FanTableEntry> gpuTable = generateTable(gpuKeyPoints);

  customFanProfile.tableCPU = cpuTable;
  customFanProfile.tableGPU = gpuTable;
  return true;
}
