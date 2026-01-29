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

#include "TccSettings.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <optional>

class SettingsManager
{
public:
  static constexpr const char *SETTINGS_FILE = "/etc/tcc/settings";
  
  [[nodiscard]] std::optional< TccSettings > readSettings() const noexcept
  {
    try
    {
      namespace fs = std::filesystem;
      
      if ( !fs::exists( SETTINGS_FILE ) )
      {
        std::cout << "[Settings] Settings file not found, using defaults" << std::endl;
        return std::nullopt;
      }
      
      std::ifstream file( SETTINGS_FILE );
      if ( !file )
      {
        std::cerr << "[Settings] Failed to open settings file" << std::endl;
        return std::nullopt;
      }
      
      std::string content( ( std::istreambuf_iterator< char >( file ) ),
                           std::istreambuf_iterator< char >() );
      
      return parseSettingsJSONInternal( content );
    }
    catch ( const std::exception &e )
    {
      std::cerr << "[Settings] Exception reading settings: " << e.what() << std::endl;
      return std::nullopt;
    }
  }
  
  [[nodiscard]] bool writeSettings( const TccSettings &settings ) const noexcept
  {
    try
    {
      namespace fs = std::filesystem;
      
      // Create directory if needed
      fs::create_directories( fs::path( SETTINGS_FILE ).parent_path() );
      
      // Serialize settings to JSON
      std::string json = settingsToJSON( settings );
      
      // Write to file
      std::ofstream file( SETTINGS_FILE );
      if ( !file )
      {
        std::cerr << "[Settings] Failed to create settings file" << std::endl;
        return false;
      }
      
      file << json;
      
      // Set permissions
      fs::permissions( SETTINGS_FILE,
                      fs::perms::owner_read | fs::perms::owner_write |
                      fs::perms::group_read | fs::perms::others_read );
      
      std::cout << "[Settings] Settings written successfully" << std::endl;
      return true;
    }
    catch ( const std::exception &e )
    {
      std::cerr << "[Settings] Exception writing settings: " << e.what() << std::endl;
      return false;
    }
  }

  // public wrapper for parsing settings JSON (used by --new_settings)
  [[nodiscard]] std::optional< TccSettings > parseSettingsJSON( const std::string &json ) const noexcept
  {
    return parseSettingsJSONInternal( json );
  }
  
private:
  [[nodiscard]] std::optional< TccSettings > parseSettingsJSONInternal( const std::string &json ) const noexcept
  {
    try
    {
      TccSettings settings;
      
      // Parse stateMap
      auto stateMapPos = json.find( "\"stateMap\"" );
      if ( stateMapPos != std::string::npos )
      {
        auto openBrace = json.find( '{', stateMapPos );
        auto closeBrace = json.find( '}', openBrace );
        
        if ( openBrace != std::string::npos && closeBrace != std::string::npos )
        {
          std::string stateMapJSON = json.substr( openBrace + 1, closeBrace - openBrace - 1 );
          
          // Parse power_ac
          auto acPos = stateMapJSON.find( "\"power_ac\"" );
          if ( acPos != std::string::npos )
          {
            auto colonPos = stateMapJSON.find( ':', acPos );
            auto quoteStart = stateMapJSON.find( '\"', colonPos );
            auto quoteEnd = stateMapJSON.find( '\"', quoteStart + 1 );
            if ( quoteStart != std::string::npos && quoteEnd != std::string::npos )
            {
              settings.stateMap["power_ac"] = stateMapJSON.substr( quoteStart + 1, quoteEnd - quoteStart - 1 );
            }
          }
          
          // Parse power_bat
          auto batPos = stateMapJSON.find( "\"power_bat\"" );
          if ( batPos != std::string::npos )
          {
            auto colonPos = stateMapJSON.find( ':', batPos );
            auto quoteStart = stateMapJSON.find( '\"', colonPos );
            auto quoteEnd = stateMapJSON.find( '\"', quoteStart + 1 );
            if ( quoteStart != std::string::npos && quoteEnd != std::string::npos )
            {
              settings.stateMap["power_bat"] = stateMapJSON.substr( quoteStart + 1, quoteEnd - quoteStart - 1 );
            }
          }
        }
      }
      
      // Parse boolean settings
      settings.fahrenheit = parseBoolField( json, "fahrenheit", false );
      settings.cpuSettingsEnabled = parseBoolField( json, "cpuSettingsEnabled", true );
      settings.fanControlEnabled = parseBoolField( json, "fanControlEnabled", true );
      settings.keyboardBacklightControlEnabled = parseBoolField( json, "keyboardBacklightControlEnabled", true );
      
      // Parse optional string fields
      settings.shutdownTime = parseOptionalStringField( json, "shutdownTime" );
      settings.chargingProfile = parseOptionalStringField( json, "chargingProfile" );
      settings.chargingPriority = parseOptionalStringField( json, "chargingPriority" );
      
      // Parse ycbcr420Workaround array
      settings.ycbcr420Workaround = parseYcbcr420Workaround( json );
      
      //std::cout << "[Settings] Loaded settings - AC: " << settings.stateMap["power_ac"] 
      //          << ", BAT: " << settings.stateMap["power_bat"] << std::endl;
      
      return settings;
    }
    catch ( const std::exception &e )
    {
      std::cerr << "[Settings] Exception parsing settings JSON: " << e.what() << std::endl;
      return std::nullopt;
    }
  }
  
  [[nodiscard]] bool parseBoolField( const std::string &json, const std::string &fieldName, bool defaultValue ) const noexcept
  {
    auto pos = json.find( "\"" + fieldName + "\"" );
    if ( pos == std::string::npos )
      return defaultValue;
    
    auto colonPos = json.find( ':', pos );
    if ( colonPos == std::string::npos )
      return defaultValue;
    
    auto truePos = json.find( "true", colonPos );
    auto falsePos = json.find( "false", colonPos );
    auto commaPos = json.find( ',', colonPos );
    auto bracePos = json.find( '}', colonPos );
    
    auto endPos = std::min( commaPos != std::string::npos ? commaPos : json.length(),
                            bracePos != std::string::npos ? bracePos : json.length() );
    
    if ( truePos != std::string::npos && truePos < endPos )
      return true;
    if ( falsePos != std::string::npos && falsePos < endPos )
      return false;
    
    return defaultValue;
  }
  
  [[nodiscard]] std::optional< std::string > parseOptionalStringField( const std::string &json, const std::string &fieldName ) const noexcept
  {
    auto pos = json.find( "\"" + fieldName + "\"" );
    if ( pos == std::string::npos )
      return std::nullopt;
    
    auto colonPos = json.find( ':', pos );
    if ( colonPos == std::string::npos )
      return std::nullopt;
    
    auto nullPos = json.find( "null", colonPos );
    auto commaPos = json.find( ',', colonPos );
    auto bracePos = json.find( '}', colonPos );
    
    auto endPos = std::min( commaPos != std::string::npos ? commaPos : json.length(),
                            bracePos != std::string::npos ? bracePos : json.length() );
    
    if ( nullPos != std::string::npos && nullPos < endPos )
      return std::nullopt;
    
    auto quoteStart = json.find( '\"', colonPos );
    auto quoteEnd = json.find( '\"', quoteStart + 1 );
    if ( quoteStart != std::string::npos && quoteEnd != std::string::npos && quoteStart < endPos )
    {
      return json.substr( quoteStart + 1, quoteEnd - quoteStart - 1 );
    }
    
    return std::nullopt;
  }
  
  [[nodiscard]] std::vector< YCbCr420Card > parseYcbcr420Workaround( const std::string &json ) const noexcept
  {
    std::vector< YCbCr420Card > result;
    
    try
    {
      // Find "ycbcr420Workaround"
      auto fieldPos = json.find( "\"ycbcr420Workaround\"" );
      if ( fieldPos == std::string::npos )
        return result;
      
      auto colonPos = json.find( ':', fieldPos );
      if ( colonPos == std::string::npos )
        return result;
      
      auto arrayStart = json.find( '[', colonPos );
      if ( arrayStart == std::string::npos )
        return result;
      
      auto arrayEnd = json.find( ']', arrayStart );
      if ( arrayEnd == std::string::npos )
        return result;
      
      // Parse array of objects
      size_t pos = arrayStart + 1;
      int cardIndex = 0;
      
      while ( pos < arrayEnd )
      {
        auto objStart = json.find( '{', pos );
        if ( objStart == std::string::npos || objStart >= arrayEnd )
          break;
        
        auto objEnd = json.find( '}', objStart );
        if ( objEnd == std::string::npos || objEnd > arrayEnd )
          break;
        
        // Parse object as card with ports
        YCbCr420Card card;
        card.card = cardIndex++;
        
        std::string objContent = json.substr( objStart + 1, objEnd - objStart - 1 );
        
        // Parse each key:value pair in object as port:enabled
        size_t pairPos = 0;
        while ( pairPos < objContent.length() )
        {
          auto keyStart = objContent.find( '\"', pairPos );
          if ( keyStart == std::string::npos )
            break;
          
          auto keyEnd = objContent.find( '\"', keyStart + 1 );
          if ( keyEnd == std::string::npos )
            break;
          
          std::string portName = objContent.substr( keyStart + 1, keyEnd - keyStart - 1 );
          
          auto colon = objContent.find( ':', keyEnd );
          if ( colon == std::string::npos )
            break;
          
          bool enabled = false;
          if ( objContent.find( "true", colon ) < objContent.find( ',', colon ) ||
               ( objContent.find( "true", colon ) != std::string::npos && objContent.find( ',', colon ) == std::string::npos ) )
          {
            enabled = true;
          }
          
          YCbCr420Port port;
          port.port = portName;
          port.enabled = enabled;
          card.ports.push_back( port );
          
          auto comma = objContent.find( ',', colon );
          if ( comma == std::string::npos )
            break;
          pairPos = comma + 1;
        }
        
        result.push_back( card );
        pos = objEnd + 1;
      }
    }
    catch ( const std::exception &e )
    {
      std::cerr << "[Settings] Exception parsing ycbcr420Workaround: " << e.what() << std::endl;
    }
    
    return result;
  }
  
  [[nodiscard]] std::string settingsToJSON( const TccSettings &settings ) const noexcept
  {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"fahrenheit\": " << ( settings.fahrenheit ? "true" : "false" ) << ",\n";
    json << "  \"stateMap\": {\n";
    json << "    \"power_ac\": \"" << settings.stateMap.at("power_ac") << "\",\n";
    json << "    \"power_bat\": \"" << settings.stateMap.at("power_bat") << "\"\n";
    json << "  },\n";
    json << "  \"shutdownTime\": " << ( settings.shutdownTime.has_value() ? "\"" + settings.shutdownTime.value() + "\"" : "null" ) << ",\n";
    json << "  \"cpuSettingsEnabled\": " << ( settings.cpuSettingsEnabled ? "true" : "false" ) << ",\n";
    json << "  \"fanControlEnabled\": " << ( settings.fanControlEnabled ? "true" : "false" ) << ",\n";
    json << "  \"keyboardBacklightControlEnabled\": " << ( settings.keyboardBacklightControlEnabled ? "true" : "false" ) << ",\n";
    
    // Serialize ycbcr420Workaround array
    json << "  \"ycbcr420Workaround\": [";
    for ( size_t cardIdx = 0; cardIdx < settings.ycbcr420Workaround.size(); ++cardIdx )
    {
      if ( cardIdx > 0 ) json << ", ";
      json << "{";
      const auto &card = settings.ycbcr420Workaround[cardIdx];
      for ( size_t portIdx = 0; portIdx < card.ports.size(); ++portIdx )
      {
        if ( portIdx > 0 ) json << ", ";
        const auto &port = card.ports[portIdx];
        json << "\"" << port.port << "\": " << ( port.enabled ? "true" : "false" );
      }
      json << "}";
    }
    json << "],\n";
    
    json << "  \"chargingProfile\": " << ( settings.chargingProfile.has_value() ? "\"" + settings.chargingProfile.value() + "\"" : "null" ) << ",\n";
    json << "  \"chargingPriority\": " << ( settings.chargingPriority.has_value() ? "\"" + settings.chargingPriority.value() + "\"" : "null" ) << ",\n";
    json << "  \"keyboardBacklightStates\": []\n";
    json << "}\n";
    
    return json.str();
  }
};
