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

#include "UccDBusService.hpp"
#include "profiles/DefaultProfiles.hpp"
#include "profiles/FanProfile.hpp"
#include "StateUtils.hpp"
#include "Utils.hpp"
#include "SysfsNode.hpp"
#include <sstream>
#include <iomanip>
#include <map>
#include <thread>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <syslog.h>
#include <libudev.h>
#include <functional>

// helper function to convert GPU info to JSON
std::string dgpuInfoToJSON( const DGpuInfo &info )
{
  std::ostringstream oss;
  oss << std::fixed << std::setprecision( 2 );
  oss << "{"
  << "\"temp\":" << info.m_temp << ","
      << "\"coreFrequency\":" << info.m_coreFrequency << ","
      << "\"maxCoreFrequency\":" << info.m_maxCoreFrequency << ","
      << "\"powerDraw\":" << info.m_powerDraw << ","
      << "\"maxPowerLimit\":" << info.m_maxPowerLimit << ","
      << "\"enforcedPowerLimit\":" << info.m_enforcedPowerLimit << ","
      << "\"d0MetricsUsage\":" << ( info.m_d0MetricsUsage ? "true" : "false" )
      << "}";
  return oss.str();
}

std::string igpuInfoToJSON( const IGpuInfo &info )
{
  std::ostringstream oss;
  oss << std::fixed << std::setprecision( 2 );
  oss << "{"
      << "\"temp\":" << info.m_temp << ","
      << "\"coreFrequency\":" << info.m_coreFrequency << ","
      << "\"maxCoreFrequency\":" << info.m_maxCoreFrequency << ","
      << "\"powerDraw\":" << info.m_powerDraw << ","
      << "\"vendor\":\"" << info.m_vendor << "\""
      << "}";
  return oss.str();
}

static std::string jsonEscape( const std::string &value )
{
  std::ostringstream oss;
  for ( const char c : value )
  {
    switch ( c )
    {
      case '"': oss << "\\\""; break;
      case '\\': oss << "\\\\"; break;
      case '\b': oss << "\\b"; break;
      case '\f': oss << "\\f"; break;
      case '\n': oss << "\\n"; break;
      case '\r': oss << "\\r"; break;
      case '\t': oss << "\\t"; break;
      default: oss << c; break;
    }
  }
  return oss.str();
}

static std::string fanTableToJSON( const std::vector< FanTableEntry > &table )
{
  std::ostringstream oss;
  oss << "[";
  for ( size_t i = 0; i < table.size(); ++i )
  {
    if ( i > 0 )
      oss << ",";

    oss << "{"
        << "\"temp\":" << table[ i ].temp << ","
        << "\"speed\":" << table[ i ].speed
        << "}";
  }
  oss << "]";
  return oss.str();
}


static int32_t getDefaultOnlineCores()
{
  const auto cores = std::thread::hardware_concurrency();
  return cores > 0 ? static_cast< int32_t >( cores ) : -1;
}

static int32_t readSysFsInt( const std::string &path, int32_t defaultValue )
{
  std::ifstream file( path );
  if ( !file.is_open() )
    return defaultValue;
  
  int32_t value;
  if ( !( file >> value ) )
    return defaultValue;
  
  return value;
}

static int32_t getCpuMinFrequency()
{
  // Read from cpu0 cpuinfo_min_freq
  return readSysFsInt( "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq", -1 );
}

static int32_t getCpuMaxFrequency()
{
  // Read from cpu0 cpuinfo_max_freq
  return readSysFsInt( "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", -1 );
}

static int32_t optionalValueOr( const std::optional< int32_t > &value, int32_t fallback )
{
  return value.has_value() ? value.value() : fallback;
}

static std::string profileToJSON( const TccProfile &profile,
                                  int32_t defaultOnlineCores,
                                  int32_t defaultScalingMin,
                                  int32_t defaultScalingMax )
{
  std::ostringstream oss;
  oss << "{"
      << "\"id\":\"" << jsonEscape( profile.id ) << "\" ,"
      << "\"name\":\"" << jsonEscape( profile.name ) << "\" ,"
      << "\"description\":\"" << jsonEscape( profile.description ) << "\" ,"
      << "\"display\":{"
      << "\"brightness\":" << profile.display.brightness << ","
      << "\"useBrightness\":" << ( profile.display.useBrightness ? "true" : "false" ) << ","
      << "\"refreshRate\":" << profile.display.refreshRate << ","
      << "\"useRefRate\":" << ( profile.display.useRefRate ? "true" : "false" ) << ","
      << "\"xResolution\":" << profile.display.xResolution << ","
      << "\"yResolution\":" << profile.display.yResolution << ","
      << "\"useResolution\":" << ( profile.display.useResolution ? "true" : "false" )
      << "},"
      << "\"cpu\":{"
      << "\"useMaxPerfGov\":" << ( profile.cpu.useMaxPerfGov ? "true" : "false" ) << ","
      << "\"governor\":\"" << jsonEscape( profile.cpu.governor ) << "\" ,"
      << "\"energyPerformancePreference\":\"" << jsonEscape( profile.cpu.energyPerformancePreference ) << "\" ,"
      << "\"noTurbo\":" << ( profile.cpu.noTurbo ? "true" : "false" ) << ","
      << "\"onlineCores\":" << optionalValueOr( profile.cpu.onlineCores, defaultOnlineCores ) << ","
      << "\"scalingMinFrequency\":" << optionalValueOr( profile.cpu.scalingMinFrequency, defaultScalingMin ) << ","
      << "\"scalingMaxFrequency\":" << optionalValueOr( profile.cpu.scalingMaxFrequency, defaultScalingMax )
      << "},"
      << "\"webcam\":{"
      << "\"status\":" << ( profile.webcam.status ? "true" : "false" ) << ","
      << "\"useStatus\":" << ( profile.webcam.useStatus ? "true" : "false" )
      << "},"
      << "\"fan\":{"
      << "\"useControl\":" << ( profile.fan.useControl ? "true" : "false" ) << ","
      << "\"fanProfile\":\"" << jsonEscape( profile.fan.fanProfile ) << "\" ,"
      << "\"minimumFanspeed\":" << profile.fan.minimumFanspeed << ","
      << "\"maximumFanspeed\":" << profile.fan.maximumFanspeed << ","
      << "\"offsetFanspeed\":" << profile.fan.offsetFanspeed
      << "},"
      << "\"odmProfile\":{"
      << "\"name\":\"" << jsonEscape( profile.odmProfile.name.value_or( "" ) ) << "\""
      << "},"
      << "\"odmPowerLimits\":{"
      << "\"tdpValues\":[";

  for ( size_t i = 0; i < profile.odmPowerLimits.tdpValues.size(); ++i )
  {
    if ( i > 0 )
      oss << ",";
    oss << profile.odmPowerLimits.tdpValues[ i ];
  }

  oss << "]}";

  if ( profile.nvidiaPowerCTRLProfile.has_value() )
  {
    oss << ",\"nvidiaPowerCTRLProfile\":{"
        << "\"cTGPOffset\":" << profile.nvidiaPowerCTRLProfile->cTGPOffset
        << "}";
  }
  else
  {
    oss << ",\"nvidiaPowerCTRLProfile\":null";
  }

  oss << "}";

  return oss.str();
}


static std::string buildSettingsJSON( const std::string &keyboardBacklightStatesJSON,
                                      const std::string &chargingProfile,
                                      const std::map< std::string, std::string > &stateMap )
{
  std::ostringstream oss;
  oss << "{"
      << "\"fahrenheit\":false,"
      << "\"stateMap\":{";
  
  // Serialize stateMap
  bool first = true;
  for ( const auto &[key, value] : stateMap )
  {
    if ( !first )
      oss << ",";
    first = false;
    oss << "\"" << jsonEscape( key ) << "\":\"" << jsonEscape( value ) << "\"";
  }
  
  oss << "},"
      << "\"shutdownTime\":null,"
      << "\"cpuSettingsEnabled\":true,"
      << "\"fanControlEnabled\":true,"
      << "\"keyboardBacklightControlEnabled\":true,"
      << "\"ycbcr420Workaround\":[],"
      << "\"chargingProfile\":\"" << jsonEscape( chargingProfile ) << "\" ,"
      << "\"chargingPriority\":null,"
      << "\"keyboardBacklightStates\":" << keyboardBacklightStatesJSON
      << "}";
  return oss.str();
}

// TccDBusInterfaceAdaptor implementation

TccDBusInterfaceAdaptor::TccDBusInterfaceAdaptor( sdbus::IObject &object,
                                                  TccDBusData &data,
                                                  UccDBusService *service )
  : m_object( object ),
    m_data( data ),
    m_service( service ),
    m_lastDataCollectionAccess( std::chrono::steady_clock::now() )
{
  registerAdaptor();
}

void TccDBusInterfaceAdaptor::registerAdaptor()
{
  using namespace sdbus;
  
  // register all methods directly on the object for sdbus-c++ 2.x
  m_object.addVTable(
    registerMethod("GetDeviceName").implementedAs([this](){ return this->GetDeviceName(); }),
    registerMethod("GetDisplayModesJSON").implementedAs([this](){ return this->GetDisplayModesJSON(); }),
    registerMethod("GetIsX11").implementedAs([this](){ return this->GetIsX11(); }),
    registerMethod("TuxedoWmiAvailable").implementedAs([this](){ return this->TuxedoWmiAvailable(); }),
    registerMethod("FanHwmonAvailable").implementedAs([this](){ return this->FanHwmonAvailable(); }),
    registerMethod("TccdVersion").implementedAs([this](){ return this->TccdVersion(); }),
    registerMethod("GetFanDataCPU").implementedAs([this](){ return this->GetFanDataCPU(); }),
    registerMethod("GetFanDataGPU1").implementedAs([this](){ return this->GetFanDataGPU1(); }),
    registerMethod("GetFanDataGPU2").implementedAs([this](){ return this->GetFanDataGPU2(); }),
    registerMethod("WebcamSWAvailable").implementedAs([this](){ return this->WebcamSWAvailable(); }),
    registerMethod("GetWebcamSWStatus").implementedAs([this](){ return this->GetWebcamSWStatus(); }),
    registerMethod("GetForceYUV420OutputSwitchAvailable").implementedAs([this](){ return this->GetForceYUV420OutputSwitchAvailable(); }),
    registerMethod("GetDGpuInfoValuesJSON").implementedAs([this](){ return this->GetDGpuInfoValuesJSON(); }),
    registerMethod("GetIGpuInfoValuesJSON").implementedAs([this](){ return this->GetIGpuInfoValuesJSON(); }),
    registerMethod("GetCpuPowerValuesJSON").implementedAs([this](){ return this->GetCpuPowerValuesJSON(); }),
    registerMethod("GetPrimeState").implementedAs([this](){ return this->GetPrimeState(); }),
    registerMethod("ConsumeModeReapplyPending").implementedAs([this](){ return this->ConsumeModeReapplyPending(); }),
    registerMethod("GetActiveProfileJSON").implementedAs([this](){ return this->GetActiveProfileJSON(); }),
    registerMethod("SetFanProfileCPU").implementedAs([this](const std::string &json){ return this->SetFanProfileCPU(json); }),
    registerMethod("SetFanProfileDGPU").implementedAs([this](const std::string &json){ return this->SetFanProfileDGPU(json); }),
    registerMethod("ApplyFanProfiles").implementedAs([this](const std::string &json){ return this->ApplyFanProfiles(json); }),
    registerMethod("RevertFanProfiles").implementedAs([this](){ return this->RevertFanProfiles(); }),
    registerMethod("SetTempProfile").implementedAs([this](const std::string &profileName){ return this->SetTempProfile(profileName); }),
    registerMethod("SetTempProfileById").implementedAs([this](const std::string &id){ return this->SetTempProfileById(id); }),
    registerMethod("SetActiveProfile").implementedAs([this](const std::string &id){ return this->SetActiveProfile(id); }),
    registerMethod("ApplyProfile").implementedAs([this](const std::string &profileJSON){ return this->ApplyProfile(profileJSON); }),

    registerMethod("GetProfilesJSON").implementedAs([this](){ return this->GetProfilesJSON(); }),
    registerMethod("GetDefaultProfilesJSON").implementedAs([this](){ return this->GetDefaultProfilesJSON(); }),
    registerMethod("GetDefaultValuesProfileJSON").implementedAs([this](){ return this->GetDefaultValuesProfileJSON(); }),
    registerMethod("CopyProfile").implementedAs([this](const std::string &sourceId, const std::string &newName){ return this->CopyProfile(sourceId, newName); }),
    registerMethod("GetFanProfile").implementedAs([this](const std::string &name){ return this->GetFanProfile(name); }),
    registerMethod("GetFanProfileNames").implementedAs([this](){ return this->GetFanProfileNames(); }),
    registerMethod("SetFanProfile").implementedAs([this](const std::string &name, const std::string &json){ return this->SetFanProfile(name, json); }),
    registerMethod("GetSettingsJSON").implementedAs([this](){ return this->GetSettingsJSON(); }),
    registerMethod("GetPowerState").implementedAs([this](){ return this->GetPowerState(); }),
    registerMethod("SetStateMap").implementedAs([this](const std::string &state, const std::string &profileId){ return this->SetStateMap(state, profileId); }),
    registerMethod("ODMProfilesAvailable").implementedAs([this](){ return this->ODMProfilesAvailable(); }),
    registerMethod("ODMPowerLimitsJSON").implementedAs([this](){ return this->ODMPowerLimitsJSON(); }),
    registerMethod("GetKeyboardBacklightCapabilitiesJSON").implementedAs([this](){ return this->GetKeyboardBacklightCapabilitiesJSON(); }),
    registerMethod("GetKeyboardBacklightStatesJSON").implementedAs([this](){ return this->GetKeyboardBacklightStatesJSON(); }),
    registerMethod("SetKeyboardBacklightStatesJSON").implementedAs([this](const std::string &json){ return this->SetKeyboardBacklightStatesJSON(json); }),
    registerMethod("GetFansMinSpeed").implementedAs([this](){ return this->GetFansMinSpeed(); }),
    registerMethod("GetFansOffAvailable").implementedAs([this](){ return this->GetFansOffAvailable(); }),
    registerMethod("GetChargingProfilesAvailable").implementedAs([this](){ return this->GetChargingProfilesAvailable(); }),
    registerMethod("GetCurrentChargingProfile").implementedAs([this](){ return this->GetCurrentChargingProfile(); }),
    registerMethod("SetChargingProfile").implementedAs([this](const std::string &profileDescriptor){ return this->SetChargingProfile(profileDescriptor); }),
    registerMethod("GetChargingPrioritiesAvailable").implementedAs([this](){ return this->GetChargingPrioritiesAvailable(); }),
    registerMethod("GetCurrentChargingPriority").implementedAs([this](){ return this->GetCurrentChargingPriority(); }),
    registerMethod("SetChargingPriority").implementedAs([this](const std::string &priorityDescriptor){ return this->SetChargingPriority(priorityDescriptor); }),
    registerMethod("GetChargeStartAvailableThresholds").implementedAs([this](){ return this->GetChargeStartAvailableThresholds(); }),
    registerMethod("GetChargeEndAvailableThresholds").implementedAs([this](){ return this->GetChargeEndAvailableThresholds(); }),
    registerMethod("GetChargeStartThreshold").implementedAs([this](){ return this->GetChargeStartThreshold(); }),
    registerMethod("GetChargeEndThreshold").implementedAs([this](){ return this->GetChargeEndThreshold(); }),
    registerMethod("SetChargeStartThreshold").implementedAs([this](int32_t value){ return this->SetChargeStartThreshold(value); }),
    registerMethod("SetChargeEndThreshold").implementedAs([this](int32_t value){ return this->SetChargeEndThreshold(value); }),
    registerMethod("GetChargeType").implementedAs([this](){ return this->GetChargeType(); }),
    registerMethod("SetChargeType").implementedAs([this](const std::string &type){ return this->SetChargeType(type); }),
    registerMethod("GetFnLockSupported").implementedAs([this](){ return this->GetFnLockSupported(); }),
    registerMethod("GetFnLockStatus").implementedAs([this](){ return this->GetFnLockStatus(); }),
    registerMethod("SetFnLockStatus").implementedAs([this](bool status){ this->SetFnLockStatus(status); }),
    registerMethod("SetSensorDataCollectionStatus").implementedAs([this](bool status){ this->SetSensorDataCollectionStatus(status); }),
    registerMethod("GetSensorDataCollectionStatus").implementedAs([this](){ return this->GetSensorDataCollectionStatus(); }),
    registerMethod("SetDGpuD0Metrics").implementedAs([this](bool status){ this->SetDGpuD0Metrics(status); }),
    registerMethod("GetNVIDIAPowerCTRLDefaultPowerLimit").implementedAs([this](){ return this->GetNVIDIAPowerCTRLDefaultPowerLimit(); }),
    registerMethod("GetNVIDIAPowerCTRLMaxPowerLimit").implementedAs([this](){ return this->GetNVIDIAPowerCTRLMaxPowerLimit(); }),
    registerMethod("GetNVIDIAPowerCTRLAvailable").implementedAs([this](){ return this->GetNVIDIAPowerCTRLAvailable(); }),
    registerSignal("ProfileChanged").withParameters<std::string>(),
    registerSignal("ModeReapplyPendingChanged").withParameters<bool>(),
    registerSignal("PowerStateChanged").withParameters<std::string>()
  ).forInterface("com.tuxedocomputers.tccd");
}

void TccDBusInterfaceAdaptor::resetDataCollectionTimeout()
{
  // Note: caller must hold m_data.dataMutex lock
  m_lastDataCollectionAccess = std::chrono::steady_clock::now();
  m_data.sensorDataCollectionStatus = true;
}

std::map< std::string, std::map< std::string, sdbus::Variant > >
TccDBusInterfaceAdaptor::exportFanData( const FanData &fanData )
{
  std::map< std::string, std::map< std::string, sdbus::Variant > > result;

  std::map< std::string, sdbus::Variant > speedData;
  speedData[ "timestamp" ] = sdbus::Variant( fanData.speed.timestamp );
  speedData[ "data" ] = sdbus::Variant( fanData.speed.data );
  result[ "speed" ] = speedData;

  std::map< std::string, sdbus::Variant > tempData;
  tempData[ "timestamp" ] = sdbus::Variant( fanData.temp.timestamp );
  tempData[ "data" ] = sdbus::Variant( fanData.temp.data );
  result[ "temp" ] = tempData;

  return result;
}

// device and system information methods

std::string TccDBusInterfaceAdaptor::GetDeviceName()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.device;
}

std::string TccDBusInterfaceAdaptor::GetDisplayModesJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.displayModes;
}

bool TccDBusInterfaceAdaptor::GetIsX11()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.isX11;
}

bool TccDBusInterfaceAdaptor::TuxedoWmiAvailable()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.tuxedoWmiAvailable;
}

bool TccDBusInterfaceAdaptor::FanHwmonAvailable()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.fanHwmonAvailable;
}

std::string TccDBusInterfaceAdaptor::TccdVersion()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.tccdVersion;
}

// fan data methods

std::map< std::string, std::map< std::string, sdbus::Variant > >
TccDBusInterfaceAdaptor::GetFanDataCPU()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  if ( m_data.fans.size() > 0 )
    return exportFanData( m_data.fans[ 0 ] );

  return {};
}

std::map< std::string, std::map< std::string, sdbus::Variant > >
TccDBusInterfaceAdaptor::GetFanDataGPU1()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  if ( m_data.fans.size() > 1 )
    return exportFanData( m_data.fans[ 1 ] );

  return {};
}

std::map< std::string, std::map< std::string, sdbus::Variant > >
TccDBusInterfaceAdaptor::GetFanDataGPU2()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  if ( m_data.fans.size() > 2 )
    return exportFanData( m_data.fans[ 2 ] );

  return {};
}

// webcam and display methods

bool TccDBusInterfaceAdaptor::WebcamSWAvailable()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.webcamSwitchAvailable;
}

bool TccDBusInterfaceAdaptor::GetWebcamSWStatus()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.webcamSwitchStatus;
}

bool TccDBusInterfaceAdaptor::GetForceYUV420OutputSwitchAvailable()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.forceYUV420OutputSwitchAvailable;
}

// gpu information methods

std::string TccDBusInterfaceAdaptor::GetDGpuInfoValuesJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  resetDataCollectionTimeout();
  return m_data.dGpuInfoValuesJSON;
}

std::string TccDBusInterfaceAdaptor::GetIGpuInfoValuesJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  resetDataCollectionTimeout();
  return m_data.iGpuInfoValuesJSON;
}

std::string TccDBusInterfaceAdaptor::GetCpuPowerValuesJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  resetDataCollectionTimeout();
  return m_data.cpuPowerValuesJSON;
}

// graphics methods

std::string TccDBusInterfaceAdaptor::GetPrimeState()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.primeState;
}

bool TccDBusInterfaceAdaptor::ConsumeModeReapplyPending()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  if ( m_data.modeReapplyPending )
  {
    m_data.modeReapplyPending = false;
    return true;
  }
  return false;
}

// profile methods

std::string TccDBusInterfaceAdaptor::GetActiveProfileJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.activeProfileJSON;
}

bool TccDBusInterfaceAdaptor::SetFanProfileCPU( const std::string &pointsJSON )
{
  if ( !m_service )
    return false;

  std::cerr << "[DBus] SetFanProfileCPU called with JSON: " << pointsJSON << std::endl;

  try
  {
    auto table = ProfileManager::parseFanTableFromJSON( pointsJSON );
    std::cerr << "[DBus] Parsed table size: " << table.size() << std::endl;
    if ( table.size() != 17 )
      return false;

    TccProfile profile = m_service->getCurrentProfile();
    std::cerr << "[DBus] Current profile ID: " << profile.id << std::endl;
    auto custom = m_service->getCustomProfiles();
    bool editable = false;
    for ( const auto &p : custom )
    {
      if ( p.id == profile.id ) { editable = true; break; }
    }
    std::cerr << "[DBus] Profile editable: " << editable << std::endl;
    if ( !editable )
      return false;

    // Apply as temporary table (do not persist in daemon profiles)
    if ( m_service->m_fanControlWorker )
    {
      m_service->m_fanControlWorker->applyTemporaryFanCurves( table, {} );
      return true;
    }
    return false;
  }
  catch ( ... )
  {
    return false;
  }
}

bool TccDBusInterfaceAdaptor::SetFanProfileDGPU( const std::string &pointsJSON )
{
  if ( !m_service )
    return false;

  std::cerr << "[DBus] SetFanProfileDGPU called with JSON: " << pointsJSON << std::endl;

  try
  {
    auto table = ProfileManager::parseFanTableFromJSON( pointsJSON );
    std::cerr << "[DBus] Parsed table size: " << table.size() << std::endl;
    if ( table.size() != 17 )
      return false;

    TccProfile profile = m_service->getCurrentProfile();
    std::cerr << "[DBus] Current profile ID: " << profile.id << std::endl;
    auto custom = m_service->getCustomProfiles();
    bool editable = false;
    for ( const auto &p : custom )
    {
      if ( p.id == profile.id ) { editable = true; break; }
    }
    std::cerr << "[DBus] Profile editable: " << editable << std::endl;
    if ( !editable )
      return false;

    // Apply as temporary table (do not persist in daemon profiles)
    if ( m_service->m_fanControlWorker )
    {
      m_service->m_fanControlWorker->applyTemporaryFanCurves( {}, table );
      return true;
    }
    return false;
  }
  catch ( ... )
  {
    return false;
  }
}

bool TccDBusInterfaceAdaptor::ApplyFanProfiles( const std::string &fanProfilesJSON )
{
  if ( !m_service )
    return false;

  std::cerr << "[DBus] ApplyFanProfiles called with JSON: " << fanProfilesJSON << std::endl;

  try
  {
    // Parse the JSON object containing cpu and gpu arrays
    // Simple parsing since we know the structure: {"cpu": [...], "gpu": [...]}
    std::string cpuJson;
    std::string gpuJson;
    
    // Extract CPU array
    size_t cpuPos = fanProfilesJSON.find("\"cpu\":");
    if ( cpuPos != std::string::npos )
    {
      size_t bracketStart = fanProfilesJSON.find( '[', cpuPos );
      if ( bracketStart != std::string::npos )
      {
        size_t bracketEnd = bracketStart;
        int depth = 0;
        for ( size_t i = bracketStart; i < fanProfilesJSON.length(); ++i )
        {
          if ( fanProfilesJSON[i] == '[' ) ++depth;
          else if ( fanProfilesJSON[i] == ']' ) --depth;
          if ( depth == 0 )
          {
            bracketEnd = i;
            break;
          }
        }
        if ( bracketEnd > bracketStart )
        {
          cpuJson = fanProfilesJSON.substr( bracketStart, bracketEnd - bracketStart + 1 );
        }
      }
    }
    
    // Extract GPU array
    size_t gpuPos = fanProfilesJSON.find("\"gpu\":");
    if ( gpuPos != std::string::npos )
    {
      size_t bracketStart = fanProfilesJSON.find( '[', gpuPos );
      if ( bracketStart != std::string::npos )
      {
        size_t bracketEnd = bracketStart;
        int depth = 0;
        for ( size_t i = bracketStart; i < fanProfilesJSON.length(); ++i )
        {
          if ( fanProfilesJSON[i] == '[' ) ++depth;
          else if ( fanProfilesJSON[i] == ']' ) --depth;
          if ( depth == 0 )
          {
            bracketEnd = i;
            break;
          }
        }
        if ( bracketEnd > bracketStart )
        {
          gpuJson = fanProfilesJSON.substr( bracketStart, bracketEnd - bracketStart + 1 );
        }
      }
    }
    
    std::vector< FanTableEntry > cpuTable;
    std::vector< FanTableEntry > gpuTable;
    
    if ( !cpuJson.empty() )
    {
      cpuTable = ProfileManager::parseFanTableFromJSON( cpuJson );
      std::cerr << "[DBus] Parsed CPU table size: " << cpuTable.size() << std::endl;
    }
    
    if ( !gpuJson.empty() )
    {
      gpuTable = ProfileManager::parseFanTableFromJSON( gpuJson );
      std::cerr << "[DBus] Parsed GPU table size: " << gpuTable.size() << std::endl;
    }
    
    // Apply the temporary fan curves
    if ( m_service->m_fanControlWorker )
    {
      m_service->m_fanControlWorker->applyTemporaryFanCurves( cpuTable, gpuTable );
      std::cerr << "[DBus] Applied temporary fan profiles" << std::endl;
    }
    
    return true;
  }
  catch ( ... )
  {
    return false;
  }
}

bool TccDBusInterfaceAdaptor::RevertFanProfiles()
{
  if ( !m_service )
    return false;

  std::cerr << "[DBus] RevertFanProfiles called" << std::endl;

  try
  {
    // Clear temporary fan curves by resetting the flag and reloading profile
    if ( m_service->m_fanControlWorker )
    {
      m_service->m_fanControlWorker->clearTemporaryCurves();
      std::cerr << "[DBus] Cleared temporary fan curves" << std::endl;
    }
    
    // Reload the current profile to reset fan logics
    auto profile = m_service->getCurrentProfile();
    // The onWork method will call updateFanLogicsFromProfile which will now use profile curves
    
    return true;
  }
  catch ( ... )
  {
    return false;
  }
}

bool TccDBusInterfaceAdaptor::SetTempProfile( const std::string &profileName )
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  m_data.tempProfileName = profileName;
  return true;
}

bool TccDBusInterfaceAdaptor::SetTempProfileById( const std::string &id )
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  m_data.tempProfileId = id;
  // trigger state check would be called here
  return true;
}

bool TccDBusInterfaceAdaptor::SetActiveProfile( const std::string &id )
{
  // Immediately set the active profile
  return m_service->setCurrentProfileById( id );
}

bool TccDBusInterfaceAdaptor::ApplyProfile( const std::string &profileJSON )
{
  // Apply the profile configuration sent by the GUI
  return m_service->applyProfileJSON( profileJSON );
}



std::string TccDBusInterfaceAdaptor::GetProfilesJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.profilesJSON;
}

std::string TccDBusInterfaceAdaptor::GetCustomProfilesJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  //std::cout << "[DBus] GetCustomProfilesJSON called, returning " 
  //          << m_data.customProfilesJSON.length() << " bytes" << std::endl;
  return m_data.customProfilesJSON;
}

std::string TccDBusInterfaceAdaptor::GetDefaultProfilesJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.defaultProfilesJSON;
}

std::string TccDBusInterfaceAdaptor::GetDefaultValuesProfileJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.defaultValuesProfileJSON;
}

bool TccDBusInterfaceAdaptor::AddCustomProfile( const std::string &profileJSON )
{
  if ( !m_service )
  {
    std::cerr << "[Profile] AddCustomProfile called but service not available" << std::endl;
    return false;
  }

  try
  {
    // Parse the profile JSON and add it
    auto profile = ProfileManager::parseProfileJSON( profileJSON );
    
    // Generate new ID if empty
    if ( profile.id.empty() )
    {
      profile.id = generateProfileId();
    }
    
    std::cout << "[Profile] Adding custom profile '" << profile.name 
              << "' (id: " << profile.id << ")" << std::endl;
    
    bool result = m_service->addCustomProfile( profile );
    
    if ( result )
    {
      std::cout << "[Profile] Successfully added profile '" << profile.name << "'" << std::endl;
    }
    else
    {
      std::cerr << "[Profile] Failed to add profile '" << profile.name << "'" << std::endl;
    }
    
    return result;
  }
  catch ( const std::exception &e )
  {
    std::cerr << "[Profile] Exception in AddCustomProfile: " << e.what() << std::endl;
    return false;
  }
  catch ( ... )
  {
    std::cerr << "[Profile] Unknown exception in AddCustomProfile" << std::endl;
    return false;
  }
}

bool TccDBusInterfaceAdaptor::DeleteCustomProfile( const std::string &profileId )
{
  if ( !m_service )
  {
    std::cerr << "[Profile] DeleteCustomProfile called but service not available" << std::endl;
    return false;
  }

  std::cout << "[Profile] Deleting custom profile with id: " << profileId << std::endl;
  
  bool result = m_service->deleteCustomProfile( profileId );
  
  if ( result )
  {
    std::cout << "[Profile] Successfully deleted profile '" << profileId << "'" << std::endl;
  }
  else
  {
    std::cerr << "[Profile] Failed to delete profile '" << profileId << "' (not found or error)" << std::endl;
  }
  
  return result;
}

bool TccDBusInterfaceAdaptor::UpdateCustomProfile( const std::string &profileJSON )
{
  if ( !m_service )
  {
    std::cerr << "[Profile] UpdateCustomProfile called but service not available" << std::endl;
    return false;
  }

  try
  {
    std::cout << "[Profile] Received profile JSON (first 200 chars): " 
              << profileJSON.substr(0, 200) << "..." << std::endl;
    
    // Parse the profile JSON and update it
    auto profile = ProfileManager::parseProfileJSON( profileJSON );
    
    if ( profile.id.empty() )
    {
      std::cerr << "[Profile] UpdateCustomProfile called with empty profile ID" << std::endl;
      return false; // Must have an ID to update
    }
    
    std::cout << "[Profile] Updating custom profile '" << profile.name 
              << "' (id: " << profile.id << ")" << std::endl;
    std::cout << "[Profile]   Fan control: " << (profile.fan.useControl ? "enabled" : "disabled") << std::endl;
    std::cout << "[Profile]   Fan profile: " << profile.fan.fanProfile << std::endl;
    std::cout << "[Profile]   Min speed: " << profile.fan.minimumFanspeed << std::endl;
    std::cout << "[Profile]   Max speed: " << profile.fan.maximumFanspeed << std::endl;
    std::cout << "[Profile]   Offset: " << profile.fan.offsetFanspeed << std::endl;
    std::cout << "[Profile]   Fan profile name: " << profile.fan.fanProfile << std::endl;
    
    bool result = m_service->updateCustomProfile( profile );
    
    if ( result )
    {
      std::cout << "[Profile] Successfully updated profile '" << profile.name << "'" << std::endl;
    }
    else
    {
      std::cerr << "[Profile] Failed to update profile '" << profile.name << "' (not found or error)" << std::endl;
    }
    
    return result;
  }
  catch ( const std::exception &e )
  {
    std::cerr << "[Profile] Exception in UpdateCustomProfile: " << e.what() << std::endl;
    return false;
  }
  catch ( ... )
  {
    std::cerr << "[Profile] Unknown exception in UpdateCustomProfile" << std::endl;
    return false;
  }
}

bool TccDBusInterfaceAdaptor::SaveCustomProfile( const std::string &profileJSON )
{
  if ( !m_service )
  {
    std::cerr << "[Profile] SaveCustomProfile called but service not available" << std::endl;
    return false;
  }

  try
  {
    std::cout << "[Profile] Received SaveCustomProfile JSON (first 200 chars): "
              << profileJSON.substr(0, 200) << "..." << std::endl;

    // Parse the profile JSON
    auto profile = ProfileManager::parseProfileJSON( profileJSON );

    if ( profile.id.empty() )
    {
      std::cout << "[Profile] SaveCustomProfile: adding new profile '" << profile.name << "'" << std::endl;
      bool result = m_service->addCustomProfile( profile );
      if ( result )
        std::cout << "[Profile] Successfully added profile '" << profile.name << "'" << std::endl;
      else
        std::cerr << "[Profile] Failed to add profile '" << profile.name << "'" << std::endl;
      return result;
    }

    std::cout << "[Profile] SaveCustomProfile: updating profile '" << profile.name << "' (id: " << profile.id << ")" << std::endl;
    bool result = m_service->updateCustomProfile( profile );

    if ( result )
      std::cout << "[Profile] Successfully updated profile '" << profile.name << "'" << std::endl;
    else
      std::cerr << "[Profile] Failed to update profile '" << profile.name << "'" << std::endl;

    return result;
  }
  catch ( const std::exception &e )
  {
    std::cerr << "[Profile] Exception in SaveCustomProfile: " << e.what() << std::endl;
    return false;
  }
  catch ( ... )
  {
    std::cerr << "[Profile] Unknown exception in SaveCustomProfile" << std::endl;
    return false;
  }
}

std::string TccDBusInterfaceAdaptor::CopyProfile( const std::string &sourceId, const std::string &newName )
{
  if ( !m_service )
  {
    std::cerr << "[Profile] CopyProfile called but service not available" << std::endl;
    return "";
  }

  std::cout << "[Profile] Copying profile '" << sourceId 
            << "' with new name '" << newName << "'" << std::endl;
  
  auto newProfile = m_service->copyProfile( sourceId, newName );
  
  if ( newProfile.has_value() )
  {
    std::cout << "[Profile] Successfully copied profile. New ID: " << newProfile->id << std::endl;
    // Return the new profile's ID
    return newProfile->id;
  }
  
  std::cerr << "[Profile] Failed to copy profile '" << sourceId << "' (not found or error)" << std::endl;
  return "";
}

std::string TccDBusInterfaceAdaptor::GetFanProfile( const std::string &name )
{
  return getFanProfileJson(name);
}

std::string TccDBusInterfaceAdaptor::GetFanProfileNames()
{
  std::vector< std::string > names;
  for (const auto &p : defaultFanProfiles) {
    names.push_back(p.name);
  }
  
  // Return as JSON array
  std::string json = "[";
  for (size_t i = 0; i < names.size(); ++i) {
    if (i > 0) json += ",";
    json += "\"" + names[i] + "\"";
  }
  json += "]";
  return json;
}

bool TccDBusInterfaceAdaptor::SetFanProfile( const std::string &name, const std::string &json )
{
  return setFanProfileJson(name, json);
}

// settings methods

std::string TccDBusInterfaceAdaptor::GetSettingsJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.settingsJSON;
}

std::string TccDBusInterfaceAdaptor::GetPowerState()
{
  // Return the current power state string (e.g. "power_ac" or "power_bat")
  try {
    // m_service owns m_currentState
    if ( m_service )
    {
      return profileStateToString( m_service->m_currentState );
    }
  }
  catch ( const std::exception &e )
  {
    std::cerr << "[DBus] GetPowerState exception: " << e.what() << std::endl;
  }
  return std::string("power_ac");
}

bool TccDBusInterfaceAdaptor::SetStateMap( const std::string &state, const std::string &profileId )
{
  if ( !m_service )
  {
    return false;
  }
  
  std::cout << "[DBus] SetStateMap: " << state << " -> " << profileId << std::endl;
  
  // Update the settings
  if ( state == "power_ac" || state == "power_bat" )
  {
    m_service->m_settings.stateMap[state] = profileId;
    m_service->updateDBusSettingsData();
    return true;
  }
  
  return false;
}

// odm methods

std::vector< std::string > TccDBusInterfaceAdaptor::ODMProfilesAvailable()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.odmProfilesAvailable;
}

std::string TccDBusInterfaceAdaptor::ODMPowerLimitsJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.odmPowerLimitsJSON;
}

// keyboard backlight methods

std::string TccDBusInterfaceAdaptor::GetKeyboardBacklightCapabilitiesJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.keyboardBacklightCapabilitiesJSON;
}

std::string TccDBusInterfaceAdaptor::GetKeyboardBacklightStatesJSON()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.keyboardBacklightStatesJSON;
}

bool TccDBusInterfaceAdaptor::SetKeyboardBacklightStatesJSON( const std::string &keyboardBacklightStatesJSON )
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  m_data.keyboardBacklightStatesNewJSON = keyboardBacklightStatesJSON;
  return true;
}

// fan control methods

int32_t TccDBusInterfaceAdaptor::GetFansMinSpeed()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.fansMinSpeed;
}

bool TccDBusInterfaceAdaptor::GetFansOffAvailable()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.fansOffAvailable;
}

// charging methods (stubs for now)

std::string TccDBusInterfaceAdaptor::GetChargingProfilesAvailable()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.chargingProfilesAvailable;
}

std::string TccDBusInterfaceAdaptor::GetCurrentChargingProfile()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.currentChargingProfile;
}

bool TccDBusInterfaceAdaptor::SetChargingProfile( const std::string &profileDescriptor )
{
  bool result = m_service->m_chargingWorker->applyChargingProfile( profileDescriptor );
  
  if ( result )
  {
    std::lock_guard< std::mutex > lock( m_data.dataMutex );
    m_data.currentChargingProfile = m_service->m_chargingWorker->getCurrentChargingProfile();
  }
  
  return result;
}

std::string TccDBusInterfaceAdaptor::GetChargingPrioritiesAvailable()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.chargingPrioritiesAvailable;
}

std::string TccDBusInterfaceAdaptor::GetCurrentChargingPriority()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.currentChargingPriority;
}

bool TccDBusInterfaceAdaptor::SetChargingPriority( const std::string &priorityDescriptor )
{
  bool result = m_service->m_chargingWorker->applyChargingPriority( priorityDescriptor );
  
  if ( result )
  {
    std::lock_guard< std::mutex > lock( m_data.dataMutex );
    m_data.currentChargingPriority = m_service->m_chargingWorker->getCurrentChargingPriority();
  }
  
  return result;
}

std::string TccDBusInterfaceAdaptor::GetChargeStartAvailableThresholds()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.chargeStartAvailableThresholds;
}

std::string TccDBusInterfaceAdaptor::GetChargeEndAvailableThresholds()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.chargeEndAvailableThresholds;
}

int32_t TccDBusInterfaceAdaptor::GetChargeStartThreshold()
{
  // Read current value directly from hardware
  return m_service->m_chargingWorker->getChargeStartThreshold();
}

int32_t TccDBusInterfaceAdaptor::GetChargeEndThreshold()
{
  // Read current value directly from hardware
  return m_service->m_chargingWorker->getChargeEndThreshold();
}

bool TccDBusInterfaceAdaptor::SetChargeStartThreshold( int32_t value )
{
  bool result = m_service->m_chargingWorker->setChargeStartThreshold( value );
  
  if ( result )
  {
    std::lock_guard< std::mutex > lock( m_data.dataMutex );
    m_data.chargeStartThreshold = value;
  }
  
  return result;
}

bool TccDBusInterfaceAdaptor::SetChargeEndThreshold( int32_t value )
{
  bool result = m_service->m_chargingWorker->setChargeEndThreshold( value );
  
  if ( result )
  {
    std::lock_guard< std::mutex > lock( m_data.dataMutex );
    m_data.chargeEndThreshold = value;
  }
  
  return result;
}

std::string TccDBusInterfaceAdaptor::GetChargeType()
{
  // Read current value directly from hardware
  return m_service->m_chargingWorker->getChargeType();
}

bool TccDBusInterfaceAdaptor::SetChargeType( const std::string &type )
{
  bool result = m_service->m_chargingWorker->setChargeType( type );
  
  if ( result )
  {
    std::lock_guard< std::mutex > lock( m_data.dataMutex );
    m_data.chargeType = type;
  }
  
  return result;
}

// fn lock methods (stubs for now)

bool TccDBusInterfaceAdaptor::GetFnLockSupported()
{
  return m_service->m_fnLockController.isSupported();
}

bool TccDBusInterfaceAdaptor::GetFnLockStatus()
{
  return m_service->m_fnLockController.getStatus();
}

void TccDBusInterfaceAdaptor::SetFnLockStatus( bool status )
{
  m_service->m_fnLockController.setStatus( status );
}

// sensor data collection methods

void TccDBusInterfaceAdaptor::SetSensorDataCollectionStatus( bool status )
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  m_data.sensorDataCollectionStatus = status;
}

bool TccDBusInterfaceAdaptor::GetSensorDataCollectionStatus()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.sensorDataCollectionStatus;
}

void TccDBusInterfaceAdaptor::SetDGpuD0Metrics( bool status )
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  m_data.d0MetricsUsage = status;
}

// nvidia power control methods

int32_t TccDBusInterfaceAdaptor::GetNVIDIAPowerCTRLDefaultPowerLimit()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.nvidiaPowerCTRLDefaultPowerLimit;
}

int32_t TccDBusInterfaceAdaptor::GetNVIDIAPowerCTRLMaxPowerLimit()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.nvidiaPowerCTRLMaxPowerLimit;
}

bool TccDBusInterfaceAdaptor::GetNVIDIAPowerCTRLAvailable()
{
  std::lock_guard< std::mutex > lock( m_data.dataMutex );
  return m_data.nvidiaPowerCTRLAvailable;
}

// signal emitters

void TccDBusInterfaceAdaptor::emitModeReapplyPendingChanged( bool pending )
{
  // signal emission to be implemented
  ( void ) pending;
}

void TccDBusInterfaceAdaptor::emitProfileChanged( const std::string &profileId )
{
  // Emit ProfileChanged signal
  m_object.emitSignal("ProfileChanged").onInterface("com.tuxedocomputers.tccd").withArguments(profileId);
}

// UccDBusService implementation

UccDBusService::UccDBusService()
  : DaemonWorker( std::chrono::milliseconds( 1000 ), false ),
    m_dbusData(),
    m_io(),
    m_connection( nullptr ),
    m_object( nullptr ),
    m_adaptor( nullptr ),
    m_started( false ),
    m_profileManager(),
    m_settingsManager(),
    m_settings(),
    m_activeProfile(),
    m_defaultProfiles(),
    m_customProfiles(),
    m_currentState( ProfileState::AC ),
    m_currentStateProfileId(),
    m_gpuWorker(),
    m_webcamWorker( m_dbusData, m_io )
{
  // set daemon version
  m_dbusData.tccdVersion = "2.1.21";
  
  // identify and set device
  auto device = identifyDevice();
  if ( device.has_value() )
  {
    m_dbusData.device = std::to_string( static_cast< int >( device.value() ) );
  }
  else
  {
    m_dbusData.device = "";
  }
  
  // detect display session type and initialize display modes
  initializeDisplayModes();
  
  // check tuxedo wmi availability
  m_dbusData.tuxedoWmiAvailable = m_io.wmiAvailable();

  // set default system JSON values (match TypeScript daemon structure)
  m_dbusData.primeState = "-1";
  m_dbusData.dGpuInfoValuesJSON = "{\"temp\":-1,\"powerDraw\":-1,\"maxPowerLimit\":-1,\"enforcedPowerLimit\":-1,\"coreFrequency\":-1,\"maxCoreFrequency\":-1}";
  m_dbusData.iGpuInfoValuesJSON = "{\"vendor\":\"unknown\",\"temp\":-1,\"coreFrequency\":-1,\"maxCoreFrequency\":-1,\"powerDraw\":-1}";
  // cpuPowerValuesJSON is set by CpuPowerWorker
  m_dbusData.nvidiaPowerCTRLAvailable = true;
  m_dbusData.nvidiaPowerCTRLDefaultPowerLimit = 95;
  m_dbusData.nvidiaPowerCTRLMaxPowerLimit = 175;
  m_dbusData.odmPowerLimitsJSON = "[{\"min\":25,\"max\":162,\"current\":162,\"descriptor\":\"pl1\"},{\"min\":25,\"max\":162,\"current\":162,\"descriptor\":\"pl2\"},{\"min\":25,\"max\":195,\"current\":195,\"descriptor\":\"pl4\"}]";
  m_dbusData.chargingProfilesAvailable = "[\"high_capacity\",\"balanced\",\"stationary\"]";
  m_dbusData.currentChargingProfile = "balanced";
  m_dbusData.chargingPrioritiesAvailable = "[]";
  m_dbusData.currentChargingPriority = "";
  m_dbusData.chargeStartAvailableThresholds = "[]";
  m_dbusData.chargeEndAvailableThresholds = "[]";
  m_dbusData.chargeStartThreshold = -1;
  m_dbusData.chargeEndThreshold = -1;
  m_dbusData.chargeType = "Unknown";
  m_dbusData.fnLockSupported = true;
  m_dbusData.fnLockStatus = false;

  // Keyboard backlight will be detected and initialized by KeyboardBacklightListener
  m_dbusData.keyboardBacklightCapabilitiesJSON = "null";
  m_dbusData.keyboardBacklightStatesJSON = "[]";
  m_dbusData.keyboardBacklightStatesNewJSON = "";
  
  // initialize profiles first (safer, doesn't start threads)
  initializeProfiles();
  
  // Load settings (creates defaults if needed)
  loadSettings();
  
  // Now build settings JSON with actual stateMap
  m_dbusData.settingsJSON = buildSettingsJSON( m_dbusData.keyboardBacklightStatesJSON,
                                               m_dbusData.currentChargingProfile,
                                               m_settings.stateMap );
  
  // Load autosave
  loadAutosave();
  
  // Initialize display backlight worker
  m_displayBacklightWorker = std::make_unique< DisplayBacklightWorker >(
    m_autosaveManager.getAutosavePath(),
    [this]() { return m_activeProfile; },
    [this]() { return m_autosave.displayBrightness; },
    [this]( int32_t brightness ) { m_autosave.displayBrightness = brightness; }
  );
  
  // initialize cpu worker
  m_cpuWorker = std::make_unique< CpuWorker >(
    [this]() { return m_activeProfile; },
    [this]() { return m_settings.cpuSettingsEnabled; },
    [this]( const std::string &msg ) { syslog( LOG_INFO, "%s", msg.c_str() ); }
  );
  
  // initialize odm power limit worker
  m_odmPowerLimitWorker = std::make_unique< ODMPowerLimitWorker >(
    &m_io,
    [this]() { return m_activeProfile; },
    [this]( const std::string &json ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      m_dbusData.odmPowerLimitsJSON = json;
    },
    [this]( const std::string &msg ) { syslog( LOG_INFO, "%s", msg.c_str() ); }
  );
  
  // initialize cpu power worker
  m_cpuPowerWorker = std::make_unique< CpuPowerWorker >(
    [this]( const std::string &json ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      m_dbusData.cpuPowerValuesJSON = json;
    },
    [this]() { return m_dbusData.sensorDataCollectionStatus; }
  );
  
  // initialize fan control worker
  m_fanControlWorker = std::make_unique< FanControlWorker >(
    &m_io,
    [this]() { return m_activeProfile; },
    [this]() { return m_settings.fanControlEnabled; },
    [this]( size_t fanIndex, int64_t timestamp, int speed ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      if ( fanIndex < m_dbusData.fans.size() )
      {
        m_dbusData.fans[fanIndex].speed.set( timestamp, speed );
      }
    },
    [this]( size_t fanIndex, int64_t timestamp, int temp ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      if ( fanIndex < m_dbusData.fans.size() )
      {
        m_dbusData.fans[fanIndex].temp.set( timestamp, temp );
      }
    }
  );
  
  // Initialize YCbCr420 workaround worker
  m_ycbcr420Worker = std::make_unique< YCbCr420WorkaroundWorker >(
    m_settings,
    m_dbusData.modeReapplyPending
  );
  
  // Update DBus availability flag
  {
    std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
    m_dbusData.forceYUV420OutputSwitchAvailable = m_ycbcr420Worker->isAvailable();
  }
  
  // Initialize NVIDIA Power Control listener on first profile set
  // Removed from constructor to prevent automatic initialization

  // Initialize Keyboard Backlight listener
  m_keyboardBacklightListener = std::make_unique< KeyboardBacklightListener >(
    [this]( const std::string &json ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      m_dbusData.keyboardBacklightCapabilitiesJSON = json;
    },
    [this]( const std::string &json ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      m_dbusData.keyboardBacklightStatesJSON = json;
    },
    [this]() -> std::string {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      return m_dbusData.keyboardBacklightStatesNewJSON;
    },
    [this]( const std::string & ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      m_dbusData.keyboardBacklightStatesNewJSON.clear();
    },
    [this]() {
      return m_settings.keyboardBacklightControlEnabled;
    }
  );
  
  // Initialize Charging worker
  m_chargingWorker = std::make_unique< ChargingWorker >();
  
  // Update DBus data with charging worker initial states
  {
    std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
    m_dbusData.chargingProfilesAvailable = m_chargingWorker->getChargingProfilesAvailableJSON();
    m_dbusData.currentChargingProfile = m_chargingWorker->getCurrentChargingProfile();
    m_dbusData.chargingPrioritiesAvailable = m_chargingWorker->getChargingPrioritiesAvailableJSON();
    m_dbusData.currentChargingPriority = m_chargingWorker->getCurrentChargingPriority();
    m_dbusData.chargeStartAvailableThresholds = m_chargingWorker->getChargeStartAvailableThresholdsJSON();
    m_dbusData.chargeEndAvailableThresholds = m_chargingWorker->getChargeEndAvailableThresholdsJSON();
    m_dbusData.chargeStartThreshold = m_chargingWorker->getChargeStartThreshold();
    m_dbusData.chargeEndThreshold = m_chargingWorker->getChargeEndThreshold();
    m_dbusData.chargeType = m_chargingWorker->getChargeType();
  }
  
  // Initialize Display Refresh Rate worker
  m_displayRefreshRateWorker = std::make_unique< DisplayRefreshRateWorker >(
    [this]() -> bool {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      return m_dbusData.isX11;
    },
    [this]( const std::string &json ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      m_dbusData.displayModes = json;
    },
    [this]( bool isX11 ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      m_dbusData.isX11 = isX11;
    },
    [this]() -> TccProfile {
      return m_activeProfile;
    }
  );
  
  // Initialize Prime worker
  m_primeWorker = std::make_unique< PrimeWorker >(
    [this]( const std::string &primeState ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      m_dbusData.primeState = primeState;
    }
  );
  
  // Initialize ODM Profile worker
  m_odmProfileWorker = std::make_unique< ODMProfileWorker >(
    &m_io,
    [this]() -> TccProfile {
      return m_activeProfile;
    },
    [this]( const std::vector< std::string > &profiles ) {
      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
      m_dbusData.odmProfilesAvailable = profiles;
    }
  );
  
  // then setup gpu callback before worker starts processing
  setupGpuDataCallback();

  // fill device-specific defaults BEFORE starting workers
  fillDeviceSpecificDefaults( m_defaultProfiles );
  fillDeviceSpecificDefaults( m_customProfiles );
  serializeProfilesJSON();

  // start worker threads after all callbacks and data are ready
  m_webcamWorker.start();
  m_gpuWorker.start();
  m_displayBacklightWorker->start();
  m_cpuWorker->start();
  m_odmPowerLimitWorker->start();
  m_cpuPowerWorker->start();
  m_fanControlWorker->start();
  m_ycbcr420Worker->start();
  if ( m_nvidiaPowerListener )
  {
    m_nvidiaPowerListener->start();
  }
  m_keyboardBacklightListener->start();
  m_chargingWorker->start();
  m_displayRefreshRateWorker->start();
  m_primeWorker->start();
  m_odmProfileWorker->start();

  // start main DBus worker loop after construction completes
  start();
}

void UccDBusService::setupGpuDataCallback()
{
  // Set up callback to update DBus data when GPU info is collected
  m_gpuWorker.setGpuDataCallback(
    [this]( const IGpuInfo &iGpuInfo, const DGpuInfo &dGpuInfo )
    {
      // safety check - ensure we're not being called during destruction
      if ( not m_started )
        return;

      std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );

      // Convert GPU structures to JSON and update DBus data
      m_dbusData.iGpuInfoValuesJSON = igpuInfoToJSON( iGpuInfo );
      m_dbusData.dGpuInfoValuesJSON = dgpuInfoToJSON( dGpuInfo );

      // Expose dGPU temperature through fan data for UI compatibility
      if ( dGpuInfo.m_temp > -1.0 and m_dbusData.fans.size() > 1 )
      {
        const auto now = std::chrono::duration_cast< std::chrono::milliseconds >(
          std::chrono::system_clock::now().time_since_epoch() ).count();

        m_dbusData.fans[ 1 ].temp.set(
          static_cast< int64_t >( now ),
          static_cast< int32_t >( std::lround( dGpuInfo.m_temp ) ) );
      }
    }
  );
}

void UccDBusService::updateFanData()
{
  int numberFans = 0;
  bool fansAvailable = m_io.getNumberFans( numberFans ) && numberFans > 0;
  
  // If getNumberFans fails, try to detect fans by reading temperature from fan 0
  if ( !fansAvailable )
  {
    int temp = -1;
    if ( m_io.getFanTemperature( 0, temp ) && temp >= 0 )
    {
      // We can read from at least fan 0, assume fans are available
      fansAvailable = true;
      numberFans = 2; // Assume CPU and GPU fans
      syslog( LOG_INFO, "UccDBusService: Detected fans by temperature reading (getNumberFans failed)" );
    }
  }

  int minSpeed = 0;
  bool fansOffAvailable = false;
  ( void ) m_io.getFansMinSpeed( minSpeed );
  ( void ) m_io.getFansOffAvailable( fansOffAvailable );

  const auto now = std::chrono::duration_cast< std::chrono::milliseconds >(
    std::chrono::system_clock::now().time_since_epoch() ).count();

  std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
  m_dbusData.fanHwmonAvailable = fansAvailable;
  m_dbusData.fansMinSpeed = minSpeed;
  m_dbusData.fansOffAvailable = fansOffAvailable;

  if ( not fansAvailable )
    return;

  const int maxFans = std::min( numberFans, static_cast< int >( m_dbusData.fans.size() ) );
  for ( int fanIndex = 0; fanIndex < maxFans; ++fanIndex )
  {
    int speedPercent = -1;
    int tempCelsius = -1;

    if ( m_io.getFanSpeedPercent( fanIndex, speedPercent ) )
    {
      m_dbusData.fans[ fanIndex ].speed.set( static_cast< int64_t >( now ), speedPercent );
    }

    if ( m_io.getFanTemperature( fanIndex, tempCelsius ) )
    {
      m_dbusData.fans[ fanIndex ].temp.set( static_cast< int64_t >( now ), tempCelsius );
    }
  }
}

void UccDBusService::onStart()
{
  if ( not m_started )
  {
    try
    {
      m_connection = sdbus::createSystemBusConnection();
      m_object = sdbus::createObject( *m_connection, sdbus::ObjectPath{ OBJECT_PATH } );
      m_adaptor = std::make_unique< TccDBusInterfaceAdaptor >( *m_object, m_dbusData, this );
      m_connection->requestName( sdbus::ServiceName{ SERVICE_NAME } );
      
      // Enter event loop asynchronously to process incoming method calls
      m_connection->enterEventLoopAsync();
      
      m_started = true;
      syslog( LOG_INFO, "DBus service started on %s", SERVICE_NAME );
    }
    catch ( const sdbus::Error &e )
    {
      syslog( LOG_ERR, "DBus service error: %s", e.what() );
      m_started = false;
    }
  }
}

void UccDBusService::onWork()
{
  if ( not m_started )
    return;

  // TODO: Remove this polling-based file modification check when the old TypeScript
  // GUI application is fully replaced. The new application should use D-Bus methods
  // (AddCustomProfile, UpdateCustomProfile, DeleteCustomProfile) instead of the
  // legacy pkexec file-write approach, making this workaround unnecessary.
  // Check if profiles file was modified externally (e.g., by old pkexec method)
  if ( m_profileManager.profilesFileModified() )
  {
    std::cout << "[ProfileManager] Detected external modification to profiles file, reloading..." << std::endl;
    initializeProfiles();
  }

  // update tuxedo wmi availability (matches typescript implementation)
  {
    std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
    m_dbusData.tuxedoWmiAvailable = m_io.wmiAvailable();
  }

  // Fan data is now updated by FanControlWorker

  // check sensor data collection timeout
  auto now = std::chrono::steady_clock::now();
  if ( m_adaptor )
  {
    std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
    auto elapsed = std::chrono::duration_cast< std::chrono::milliseconds >(
      now - m_adaptor->m_lastDataCollectionAccess ).count();

    if ( elapsed > 10000 )
      m_dbusData.sensorDataCollectionStatus = false;
  }

  // STATE-BASED PROFILE SWITCHING (like TypeScript StateSwitcherWorker)
  // Disabled: tccd-ng no longer saves or monitors settings file
  // UCC handles all profile decisions
  
  // Monitor power state and emit signals for UCC to handle
  {
    const ProfileState newState = determineState();
    const std::string stateKey = profileStateToString( newState );

    if ( newState != m_currentState )
    {
      m_currentState = newState;
      m_currentStateProfileId = m_settings.stateMap[stateKey];
      
      std::cout << "[State] Power state changed to " << stateKey << std::endl;
      
      // Emit signal for UCC to handle profile switching
      m_object->emitSignal("PowerStateChanged").onInterface("com.tuxedocomputers.tccd").withArguments(stateKey);
    }
  }
  
  // Monitor power state and emit signals for UCC to handle
  {
    const ProfileState newState = determineState();
    const std::string stateKey = profileStateToString( newState );

    if ( newState != m_currentState )
    {
      m_currentState = newState;
      m_currentStateProfileId = m_settings.stateMap[stateKey];
      
      std::cout << "[State] Power state changed to " << stateKey << std::endl;
      
      // Emit signal for UCC to handle profile switching
      m_object->emitSignal("PowerStateChanged").onInterface("com.tuxedocomputers.tccd").withArguments(stateKey);
    }
  }
  
  // Check for temp profile requests
  const std::string oldActiveProfileId = m_activeProfile.id;
  const std::string oldActiveProfileName = m_activeProfile.name;
  
  // Check if a temp profile by ID was requested
  std::string profileId;
  {
    std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
    if ( m_dbusData.tempProfileId.empty() || m_dbusData.tempProfileId == oldActiveProfileId )
    {
      profileId.clear();
    }
    else
    {
      profileId = m_dbusData.tempProfileId;
      m_dbusData.tempProfileId.clear(); // Clear before applying
    }
  }

  if ( !profileId.empty() )
  {
    std::cout << "[Profile] Applying temp profile by ID: " << profileId << std::endl;
    if ( setCurrentProfileById( profileId ) )
    {
      std::cout << "[Profile] Successfully switched to profile ID: " << profileId << std::endl;
    }
    else
    {
      std::cerr << "[Profile] Failed to switch to profile ID: " << profileId << std::endl;
    }
    
    return; // Process one change per cycle
  }
  
  // Check if a temp profile by name was requested
  std::string profileName;
  {
    std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
    if ( m_dbusData.tempProfileName.empty() || m_dbusData.tempProfileName == oldActiveProfileName )
    {
      profileName.clear();
    }
    else
    {
      profileName = m_dbusData.tempProfileName;
      m_dbusData.tempProfileName.clear(); // Clear before applying
    }
  }

  if ( !profileName.empty() )
  {
    std::cout << "[Profile] Applying temp profile by name: " << profileName << std::endl;
    if ( setCurrentProfileByName( profileName ) )
    {
      std::cout << "[Profile] Successfully switched to profile: " << profileName << std::endl;
    }
    else
    {
      std::cerr << "[Profile] Failed to switch to profile: " << profileName << std::endl;
    }
    
    return; // Process one change per cycle
  }

  // emit signal if mode reapply is pending
  {
    std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
    if ( m_dbusData.modeReapplyPending and m_adaptor )
      m_adaptor->emitModeReapplyPendingChanged( true );
  }
}

void UccDBusService::onExit()
{
  // Save autosave data
  saveAutosave();
  
  if ( m_started and m_connection )
  {
    try
    {
      // Leave event loop before releasing resources
      m_connection->leaveEventLoop();
      
      m_connection->releaseName( sdbus::ServiceName{ SERVICE_NAME } );
      std::cout << "dbus service stopped" << std::endl;
    }
    catch ( const sdbus::Error &e )
    {
      syslog( LOG_ERR, "DBus service error on exit: %s", e.what() );
    }
  }

  m_adaptor.reset();
  m_object.reset();
  m_connection.reset();
  m_started = false;
}

// profile management implementation

void UccDBusService::loadProfiles()
{
  std::cout << "[ProfileManager] Loading profiles..." << std::endl;
  
  // identify device for device-specific profiles
  auto device = identifyDevice();
  
  // load default profiles
  m_defaultProfiles = m_profileManager.getDefaultProfiles( device );
  std::cout << "[ProfileManager] Loaded " << m_defaultProfiles.size() << " default profiles" << std::endl;

  // load custom profiles from /etc/tcc/profiles
  m_customProfiles = m_profileManager.getCustomProfilesNoThrow();
  
  // If no custom profiles loaded (file doesn't exist or error), create defaults
  if ( m_customProfiles.empty() )
  {
    std::cout << "[ProfileManager] No custom profiles found, creating defaults" << std::endl;
    m_customProfiles = m_profileManager.getDefaultCustomProfiles();
    // Try to write defaults to disk (ignore if it fails)
    if ( m_profileManager.writeProfiles( m_customProfiles ) )
    {
      std::cout << "[ProfileManager] Default custom profiles written to " 
                << m_profileManager.getProfilesPath() << std::endl;
    }
    else
    {
      std::cerr << "[ProfileManager] Failed to write default custom profiles to disk" << std::endl;
    }
  }
  else
  {
    std::cout << "[ProfileManager] Loaded " << m_customProfiles.size() 
              << " custom profiles from " << m_profileManager.getProfilesPath() << std::endl;
  }
  
  // Fill device-specific defaults (TDP values, etc.) after loading profiles
  fillDeviceSpecificDefaults( m_defaultProfiles );
  fillDeviceSpecificDefaults( m_customProfiles );
  
  // Debug: Verify TDP values were filled
  std::cout << "[loadProfiles] After fillDeviceSpecificDefaults, checking TDP values:" << std::endl;
  for ( size_t i = 0; i < m_defaultProfiles.size() && i < 3; ++i )
  {
    std::cout << "[loadProfiles]   Default profile " << i << " (" << m_defaultProfiles[i].id 
              << ") has " << m_defaultProfiles[i].odmPowerLimits.tdpValues.size() << " TDP values";
    if ( !m_defaultProfiles[i].odmPowerLimits.tdpValues.empty() )
    {
      std::cout << ": [";
      for ( size_t j = 0; j < m_defaultProfiles[i].odmPowerLimits.tdpValues.size(); ++j )
      {
        if ( j > 0 ) std::cout << ", ";
        std::cout << m_defaultProfiles[i].odmPowerLimits.tdpValues[j];
      }
      std::cout << "]";
    }
    std::cout << std::endl;
  }
}

void UccDBusService::initializeProfiles()
{
  loadProfiles();

  // Don't set any active profile on startup - let UCC handle this
  // Only refresh if we already have an active profile (from autosave)
  if ( !m_activeProfile.id.empty() )
  {
    // Refresh the active profile from the reloaded profiles
    // in case it was modified
    std::string currentId = m_activeProfile.id;
    if ( !setCurrentProfileById( currentId ) )
    {
      // Profile no longer exists, clear it
      m_activeProfile = TccProfile();
    }
  }

  // update dbus data with profile JSONs
  updateDBusActiveProfileData();

  const int32_t defaultOnlineCores = getDefaultOnlineCores();
  const int32_t defaultScalingMin = getCpuMinFrequency();
  const int32_t defaultScalingMax = getCpuMaxFrequency();
  
  TccProfile baseCustomProfile = m_profileManager.getDefaultCustomProfiles()[0];
  
  // serialize all profiles to JSON
  std::ostringstream allProfilesJSON;
  allProfilesJSON << "[";
  
  auto allProfiles = getAllProfiles();
  for ( size_t i = 0; i < allProfiles.size(); ++i )
  {
    if ( i > 0 )
      allProfilesJSON << ",";
    
    allProfilesJSON << profileToJSON( allProfiles[ i ],
                      defaultOnlineCores,
                      defaultScalingMin,
                      defaultScalingMax );
  }
  allProfilesJSON << "]";

  std::ostringstream defaultProfilesJSON;
  defaultProfilesJSON << "[";
  for ( size_t i = 0; i < m_defaultProfiles.size(); ++i )
  {
    if ( i > 0 )
      defaultProfilesJSON << ",";
    
    defaultProfilesJSON << profileToJSON( m_defaultProfiles[ i ],
                        defaultOnlineCores,
                        defaultScalingMin,
                        defaultScalingMax );
  }
  defaultProfilesJSON << "]";

  std::ostringstream customProfilesJSON;
  customProfilesJSON << "[";
  for ( size_t i = 0; i < m_customProfiles.size(); ++i )
  {
    if ( i > 0 )
      customProfilesJSON << ",";
    
    customProfilesJSON << profileToJSON( m_customProfiles[ i ],
                       defaultOnlineCores,
                       defaultScalingMin,
                       defaultScalingMax );
  }
  customProfilesJSON << "]";

  std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
  m_dbusData.profilesJSON = defaultProfilesJSON.str();  // Only default profiles now
  m_dbusData.defaultProfilesJSON = defaultProfilesJSON.str();
  m_dbusData.customProfilesJSON = "[]";  // Empty array since custom profiles are local
  m_dbusData.defaultValuesProfileJSON = profileToJSON( baseCustomProfile,
                                                       defaultOnlineCores,
                                                       defaultScalingMin,
                                                       defaultScalingMax );
  
  std::cout << "[DBus] Updated profile JSONs:" << std::endl;
  std::cout << "[DBus]   customProfilesJSON: " << m_dbusData.customProfilesJSON.length() << " bytes, " 
            << m_customProfiles.size() << " profiles" << std::endl;
  std::cout << "[DBus]   defaultProfilesJSON: " << m_dbusData.defaultProfilesJSON.length() << " bytes, " 
            << m_defaultProfiles.size() << " profiles" << std::endl;
  
}

TccProfile UccDBusService::getCurrentProfile() const
{
  return m_activeProfile;
}

bool UccDBusService::setCurrentProfileByName( const std::string &profileName )
{
  auto allProfiles = getAllProfiles();
  
  for ( const auto &profile : allProfiles )
  {
    if ( profile.name == profileName )
    {
      m_activeProfile = profile;
      updateDBusActiveProfileData();
      return true;
    }
  }

  // fallback to default profile
  m_activeProfile = getDefaultProfile();
  updateDBusActiveProfileData();
  return false;
}

bool UccDBusService::setCurrentProfileById( const std::string &id )
{
  auto allProfiles = getAllProfiles();
  
  for ( const auto &profile : allProfiles )
  {
    if ( profile.id == id )
    {
      std::cout << "[Profile] Switching to profile: " << profile.name << " (ID: " << id << ")" << std::endl;
      m_activeProfile = profile;
      updateDBusActiveProfileData();
      
      // apply new profile to workers
      if ( m_cpuWorker )
      {
        std::cout << "[Profile] Applying CPU settings from profile" << std::endl;
        m_cpuWorker->reapplyProfile();
      }
      if ( m_odmPowerLimitWorker )
      {
        std::cout << "[Profile] Applying TDP settings from profile" << std::endl;
        m_odmPowerLimitWorker->reapplyProfile();
      }
      if ( m_nvidiaPowerListener && m_nvidiaPowerListener->isAvailable() )
      {
        std::cout << "[Profile] Notifying NVIDIA power control listener" << std::endl;
        m_nvidiaPowerListener->onActiveProfileChanged();
      }
      else if ( !m_nvidiaPowerListener )
      {
        // Initialize NVIDIA Power Control listener on first profile set
        m_nvidiaPowerListener = std::make_unique< NVIDIAPowerCTRLListener >(
          [this]() { return m_activeProfile; },
          m_dbusData.nvidiaPowerCTRLDefaultPowerLimit,
          m_dbusData.nvidiaPowerCTRLMaxPowerLimit,
          m_dbusData.nvidiaPowerCTRLAvailable
        );
        if ( m_nvidiaPowerListener && m_nvidiaPowerListener->isAvailable() )
        {
          std::cout << "[Profile] Notifying NVIDIA power control listener" << std::endl;
          m_nvidiaPowerListener->onActiveProfileChanged();
        }
      }
      
      // Emit ProfileChanged signal for DBus clients
      if ( m_adaptor )
      {
        m_adaptor->emitProfileChanged( id );
      }
      
      return true;
    }
  }

  // fallback to default profile
  std::cout << "[Profile] Profile ID not found: " << id << ", using default" << std::endl;
  m_activeProfile = getDefaultProfile();
  updateDBusActiveProfileData();
  
  // Emit ProfileChanged signal for DBus clients
  if ( m_adaptor )
  {
    m_adaptor->emitProfileChanged( m_activeProfile.id );
  }
  
  return false;
}

bool UccDBusService::applyProfileJSON( const std::string &profileJSON )
{
  try
  {
    // Parse the profile JSON
    auto profile = m_profileManager.parseProfileJSON( profileJSON );
    
    std::cout << "[Profile] Applying profile from GUI: " << profile.name << std::endl;
    
    // Set as active profile
    m_activeProfile = profile;
    updateDBusActiveProfileData();

    // If the profile explicitly sets sameSpeed, apply it to fan worker immediately
    try
    {
      if ( m_fanControlWorker )
      {
        bool same = m_activeProfile.fan.sameSpeed;
        m_fanControlWorker->setSameSpeed( same );
        syslog( LOG_INFO, "UccDBusService: applied sameSpeed=%d from profile", same ? 1 : 0 );
      }
    }
    catch ( ... ) { /* ignore */ }

    // Try to resolve and apply a built-in fan profile referenced by the profile
    try
    {
      const std::string fpName = profile.fan.fanProfile;
      if ( !fpName.empty() )
      {
        std::string fanJson = getFanProfileJson( fpName );
        if ( fanJson != "{}" )
        {
          // Extract CPU/GPU arrays from the fan JSON. Support both "tableCPU"/"tableGPU" and "cpu"/"gpu" names.
          std::string cpuJson;
          std::string gpuJson;

          auto extractArrayByKey = [&]( const std::string &key )->std::string {
            std::string keyStr1 = "\"" + key + "\":";
            // fallback to short name (remove 'table' prefix if present)
            std::string keyStr2;
            if ( key.rfind("table", 0) == 0 && key.size() > 5 )
              keyStr2 = "\"" + key.substr(5) + "\":"; // e.g. "cpu":

            size_t pos = fanJson.find( keyStr1 );
            if ( pos == std::string::npos && !keyStr2.empty() ) pos = fanJson.find( keyStr2 );
            if ( pos == std::string::npos ) return std::string();

            size_t bracketStart = fanJson.find( '[', pos );
            if ( bracketStart == std::string::npos ) return std::string();

            size_t bracketEnd = bracketStart;
            int depth = 0;
            for ( size_t i = bracketStart; i < fanJson.length(); ++i )
            {
              if ( fanJson[i] == '[' ) ++depth;
              else if ( fanJson[i] == ']' ) --depth;
              if ( depth == 0 ) { bracketEnd = i; break; }
            }
            if ( bracketEnd > bracketStart ) return fanJson.substr( bracketStart, bracketEnd - bracketStart + 1 );
            return std::string();
          };

          cpuJson = extractArrayByKey( "tableCPU" );
          if ( cpuJson.empty() ) cpuJson = extractArrayByKey( "cpu" );

          gpuJson = extractArrayByKey( "tableGPU" );
          if ( gpuJson.empty() ) gpuJson = extractArrayByKey( "gpu" );

          std::vector< FanTableEntry > cpuTable;
          std::vector< FanTableEntry > gpuTable;

          if ( !cpuJson.empty() ) cpuTable = ProfileManager::parseFanTableFromJSON( cpuJson );
          if ( !gpuJson.empty() ) gpuTable = ProfileManager::parseFanTableFromJSON( gpuJson );

          if ( m_cpuWorker )
          {
            if ( m_fanControlWorker )
            {
              m_fanControlWorker->applyTemporaryFanCurves( cpuTable, gpuTable );
            }
          }
        }
      }
    }
    catch ( ... ) { /* ignore parse errors and continue applying other profile settings */ }

    // Apply to workers
    if ( m_cpuWorker )
    {
      std::cout << "[Profile] Applying CPU settings from profile" << std::endl;
      m_cpuWorker->reapplyProfile();
    }
    if ( m_odmPowerLimitWorker )
    {
      std::cout << "[Profile] Applying TDP settings from profile" << std::endl;
      m_odmPowerLimitWorker->reapplyProfile();
    }
    
    // Emit ProfileChanged signal for DBus clients
    if ( m_adaptor )
    {
      m_adaptor->emitProfileChanged( profile.id );
    }
    
    return true;
  }
  catch ( const std::exception &e )
  {
    std::cerr << "[Profile] Failed to apply profile JSON: " << e.what() << std::endl;
    return false;
  }
}


std::vector< TccProfile > UccDBusService::getAllProfiles() const
{
  std::vector< TccProfile > allProfiles;
  allProfiles.reserve( m_defaultProfiles.size() + m_customProfiles.size() );
  
  allProfiles.insert( allProfiles.end(), m_defaultProfiles.begin(), m_defaultProfiles.end() );
  allProfiles.insert( allProfiles.end(), m_customProfiles.begin(), m_customProfiles.end() );
  
  return allProfiles;
}

std::vector< TccProfile > UccDBusService::getDefaultProfiles() const
{
  return m_defaultProfiles;
}

std::vector< TccProfile > UccDBusService::getCustomProfiles() const
{
  return m_customProfiles;
}

TccProfile UccDBusService::getDefaultProfile() const
{
  if ( not m_defaultProfiles.empty() )
    return m_defaultProfiles[0];
  
  if ( not m_customProfiles.empty() )
    return m_customProfiles[0];
  
  // ultimate fallback
  return defaultCustomProfile;
}

void UccDBusService::updateDBusActiveProfileData()
{
  const int32_t defaultOnlineCores = getDefaultOnlineCores();
  const int32_t defaultScalingMin = -1;
  const int32_t defaultScalingMax = -1;

  std::string profileJSON = profileToJSON( m_activeProfile,
                                           defaultOnlineCores,
                                           defaultScalingMin,
                                           defaultScalingMax );
  std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
  m_dbusData.activeProfileJSON = profileJSON;
}

void UccDBusService::updateDBusSettingsData()
{
  std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
  m_dbusData.settingsJSON = buildSettingsJSON( m_dbusData.keyboardBacklightStatesJSON,
                                               m_dbusData.currentChargingProfile,
                                               m_settings.stateMap );
}

bool UccDBusService::addCustomProfile( const TccProfile &profile )
{
  std::cout << "[ProfileManager] Adding profile '" << profile.name << "' to memory" << std::endl;
  
  // Add to in-memory profiles
  m_customProfiles.push_back( profile );
  
  // Update DBus data
  updateDBusActiveProfileData();
  
  std::cout << "[ProfileManager] Profile added successfully" << std::endl;
  return true;
}

bool UccDBusService::deleteCustomProfile( const std::string &profileId )
{
  std::cout << "[ProfileManager] Deleting profile '" << profileId << "' from memory" << std::endl;
  
  // Remove from in-memory profiles
  auto it = std::remove_if( m_customProfiles.begin(), m_customProfiles.end(),
                           [&profileId]( const TccProfile &p ) { return p.id == profileId; } );
  
  if ( it != m_customProfiles.end() )
  {
    m_customProfiles.erase( it, m_customProfiles.end() );
    
    // Update DBus data
    updateDBusActiveProfileData();
    
    std::cout << "[ProfileManager] Profile deleted successfully" << std::endl;
    return true;
  }
  
  std::cerr << "[ProfileManager] Profile not found" << std::endl;
  return false;
}

bool UccDBusService::updateCustomProfile( const TccProfile &profile )
{
  std::cout << "[ProfileManager] Updating profile '" << profile.name << "' in memory" << std::endl;
  
  // Check if this is a default (hardcoded) profile
  bool isDefaultProfile = false;
  for ( const auto &defaultProf : m_defaultProfiles )
  {
    if ( defaultProf.id == profile.id )
    {
      isDefaultProfile = true;
      break;
    }
  }
  
  if ( isDefaultProfile )
  {
    std::cout << "[ProfileManager] Cannot update hardcoded default profile '" << profile.id << "'" << std::endl;
    std::cout << "[ProfileManager] Default profiles are read-only." << std::endl;
    std::cerr << "[ProfileManager] ERROR: Attempt to modify read-only default profile rejected!" << std::endl;
    return false;
  }
  
  // Update in-memory profile
  auto it = std::find_if( m_customProfiles.begin(), m_customProfiles.end(),
                         [&profile]( const TccProfile &p ) { return p.id == profile.id; } );
  
  if ( it != m_customProfiles.end() )
  {
    *it = profile;
    
    // Update DBus data
    updateDBusActiveProfileData();
    
    // Update active profile if it was the one modified
    if ( m_activeProfile.id == profile.id )
    {
      std::cout << "[ProfileManager] Updated profile is active, reapplying to system" << std::endl;
      m_activeProfile = profile;
      // Reapply the profile to actually update the hardware/system settings
      if ( setCurrentProfileById( profile.id ) )
      {
        std::cout << "[ProfileManager] Successfully reapplied updated profile to system" << std::endl;
      }
      else
      {
        std::cerr << "[ProfileManager] Failed to reapply updated profile!" << std::endl;
      }
    }
    
    std::cout << "[ProfileManager] Profile updated successfully" << std::endl;
    return true;
  }
  
  std::cerr << "[ProfileManager] Profile not found for update" << std::endl;
  return false;
}

std::optional< TccProfile > UccDBusService::copyProfile( const std::string &sourceId, const std::string &newName )
{
  std::cout << "[ProfileManager] Copying profile '" << sourceId 
            << "' with new name '" << newName << "'" << std::endl;
  
  auto newProfile = m_profileManager.copyProfile( sourceId, newName );
  
  if ( newProfile.has_value() )
  {
    std::cout << "[ProfileManager] Profile copied successfully (new ID: " << newProfile->id 
              << "), reloading all profiles" << std::endl;
    // Reload profiles and update DBus data
    initializeProfiles();
  }
  else
  {
    std::cerr << "[ProfileManager] Failed to copy profile" << std::endl;
  }
  
  return newProfile;
}

void UccDBusService::initializeDisplayModes()
{
  // detect session type (x11 or wayland)
  std::string sessionType = TccUtils::executeCommand( 
    "cat $(printf \"/proc/%s/environ \" $(pgrep -vu root | tail -n 20)) 2>/dev/null | "
    "tr '\\0' '\\n' | grep -m1 '^XDG_SESSION_TYPE=' | cut -d= -f2" 
  );
  
  // trim whitespace
  while ( not sessionType.empty() and 
          ( sessionType.back() == '\n' or sessionType.back() == '\r' or 
            sessionType.back() == ' ' or sessionType.back() == '\t' ) )
  {
    sessionType.pop_back();
  }
  
  m_dbusData.isX11 = ( sessionType == "x11" );
  
  // initialize display modes as empty array - will be populated by display worker if implemented
  // must be valid JSON (empty array, not empty string) for GUI to parse correctly
  m_dbusData.displayModes = "[]";
}

std::optional< TUXEDODevice > UccDBusService::identifyDevice()
{
  // read dmi information from sysfs
  const std::string dmiBasePath = "/sys/class/dmi/id";
  const std::string productSKU = SysfsNode< std::string >( dmiBasePath + "/product_sku" ).read().value_or( "" );
  const std::string boardName = SysfsNode< std::string >( dmiBasePath + "/board_name" ).read().value_or( "" );

  // get module info from tuxedo_io
  std::string deviceModelId;
  m_io.deviceModelIdStr( deviceModelId );

  // create dmi sku to device map (matches typescript version)
  std::map< std::string, TUXEDODevice > dmiSKUDeviceMap;
  dmiSKUDeviceMap[ "IBS1706" ] = TUXEDODevice::IBP17G6;
  dmiSKUDeviceMap[ "IBP1XI08MK1" ] = TUXEDODevice::IBPG8;
  dmiSKUDeviceMap[ "IBP1XI08MK2" ] = TUXEDODevice::IBPG8;
  dmiSKUDeviceMap[ "IBP14I08MK2" ] = TUXEDODevice::IBPG8;
  dmiSKUDeviceMap[ "IBP16I08MK2" ] = TUXEDODevice::IBPG8;
  dmiSKUDeviceMap[ "OMNIA08IMK2" ] = TUXEDODevice::IBPG8;
  dmiSKUDeviceMap[ "IBP14A10MK1 / IBP15A10MK1" ] = TUXEDODevice::IBPG10AMD;
  dmiSKUDeviceMap[ "IIBP14A10MK1 / IBP15A10MK1" ] = TUXEDODevice::IBPG10AMD;
  dmiSKUDeviceMap[ "POLARIS1XA02" ] = TUXEDODevice::POLARIS1XA02;
  dmiSKUDeviceMap[ "POLARIS1XI02" ] = TUXEDODevice::POLARIS1XI02;
  dmiSKUDeviceMap[ "POLARIS1XA03" ] = TUXEDODevice::POLARIS1XA03;
  dmiSKUDeviceMap[ "POLARIS1XI03" ] = TUXEDODevice::POLARIS1XI03;
  dmiSKUDeviceMap[ "STELLARIS1XA03" ] = TUXEDODevice::STELLARIS1XA03;
  dmiSKUDeviceMap[ "STEPOL1XA04" ] = TUXEDODevice::STEPOL1XA04;
  dmiSKUDeviceMap[ "STELLARIS1XI03" ] = TUXEDODevice::STELLARIS1XI03;
  dmiSKUDeviceMap[ "STELLARIS1XI04" ] = TUXEDODevice::STELLARIS1XI04;
  dmiSKUDeviceMap[ "PULSE1502" ] = TUXEDODevice::PULSE1502;
  dmiSKUDeviceMap[ "PULSE1403" ] = TUXEDODevice::PULSE1403;
  dmiSKUDeviceMap[ "PULSE1404" ] = TUXEDODevice::PULSE1404;
  dmiSKUDeviceMap[ "STELLARIS1XI05" ] = TUXEDODevice::STELLARIS1XI05;
  dmiSKUDeviceMap[ "POLARIS1XA05" ] = TUXEDODevice::POLARIS1XA05;
  dmiSKUDeviceMap[ "STELLARIS1XA05" ] = TUXEDODevice::STELLARIS1XA05;
  dmiSKUDeviceMap[ "STELLARIS16I06" ] = TUXEDODevice::STELLARIS16I06;
  dmiSKUDeviceMap[ "STELLARIS17I06" ] = TUXEDODevice::STELLARIS17I06;
  dmiSKUDeviceMap[ "STELLSL15A06" ] = TUXEDODevice::STELLSL15A06;
  dmiSKUDeviceMap[ "STELLSL15I06" ] = TUXEDODevice::STELLSL15I06;
  dmiSKUDeviceMap[ "AURA14GEN3" ] = TUXEDODevice::AURA14G3;
  dmiSKUDeviceMap[ "AURA15GEN3" ] = TUXEDODevice::AURA15G3;
  dmiSKUDeviceMap[ "STELLARIS16A07" ] = TUXEDODevice::STELLARIS16A07;
  dmiSKUDeviceMap[ "STELLARIS16I07" ] = TUXEDODevice::STELLARIS16I07;
  dmiSKUDeviceMap[ "XNE16A25" ] = TUXEDODevice::STELLARIS16A07;
  dmiSKUDeviceMap[ "XNE16E25" ] = TUXEDODevice::STELLARIS16I07;
  dmiSKUDeviceMap[ "SIRIUS1601" ] = TUXEDODevice::SIRIUS1601;
  dmiSKUDeviceMap[ "SIRIUS1602" ] = TUXEDODevice::SIRIUS1602;

  // check for sku match
  auto skuIt = dmiSKUDeviceMap.find( productSKU );
  if ( skuIt != dmiSKUDeviceMap.end() )
  {
    return skuIt->second;
  }

  // check uwid (univ wmi interface) device mapping
  std::map< int, TUXEDODevice > uwidDeviceMap;
  uwidDeviceMap[ 0x13 ] = TUXEDODevice::IBP14G6_TUX;
  uwidDeviceMap[ 0x12 ] = TUXEDODevice::IBP14G6_TRX;
  uwidDeviceMap[ 0x14 ] = TUXEDODevice::IBP14G6_TQF;
  uwidDeviceMap[ 0x17 ] = TUXEDODevice::IBP14G7_AQF_ARX;

  int modelId = 0;
  try
  {
    modelId = std::stoi( deviceModelId );
  }
  catch ( ... )
  {
    // ignore parse errors
  }

  auto uwidIt = uwidDeviceMap.find( modelId );
  if ( uwidIt != uwidDeviceMap.end() )
  {
    return uwidIt->second;
  }

  // no device match found
  return std::nullopt;
}

void UccDBusService::loadSettings()
{
  auto loadedSettings = m_settingsManager.readSettings();
  
  if ( loadedSettings.has_value() )
  {
    m_settings = *loadedSettings;
    std::cout << "[Settings] Loaded existing settings" << std::endl;
  }
  else
  {
    // Use defaults
    m_settings = TccSettings();
    
    // Set both AC and battery to the default custom profile
    auto allProfiles = getAllProfiles();
    if ( !allProfiles.empty() )
    {
      m_settings.stateMap["power_ac"] = allProfiles[0].id;
      m_settings.stateMap["power_bat"] = allProfiles[0].id;
    }
    
    // Write default settings
    if ( m_settingsManager.writeSettings( m_settings ) )
    {
      std::cout << "[Settings] Created default settings" << std::endl;
      updateDBusSettingsData();
    }
    else
    {
      std::cerr << "[Settings] Failed to write default settings!" << std::endl;
    }
  }
  
  // validate and fix state map if needed
  auto allProfiles = getAllProfiles();
  bool settingsChanged = false;
  
  for ( const auto &stateKey : { "power_ac", "power_bat" } )
  {
    // check if state key exists in map
    if ( m_settings.stateMap.find( stateKey ) == m_settings.stateMap.end() )
    {
      std::cout << "[Settings] Missing state id assignment for '" 
                << stateKey << "' default to + '__default_custom_profile__'" << std::endl;

      if ( not allProfiles.empty() )
      {
        m_settings.stateMap[stateKey] = allProfiles[0].id;
        settingsChanged = true;
      }

      continue;
    }

    auto &profileId = m_settings.stateMap[stateKey];
    
    // check if assigned profile exists
    bool profileExists = false;

    for ( const auto &profile : allProfiles )
    {
      if ( profile.id == profileId )
      {
        profileExists = true;
        break;
      }
    }
    
    if ( not profileExists )
    {
      std::cout << "[Settings] Profile ID '" << profileId << "' for state '" 
                << stateKey << "' not found, resetting to default" << std::endl;
      
      if ( not allProfiles.empty() )
      {
        profileId = allProfiles[0].id;
        settingsChanged = true;
      }
    }
  }
  
  if ( settingsChanged )
  {
    if ( m_settingsManager.writeSettings( m_settings ) )
    {
      std::cout << "[Settings] Saved updated settings" << std::endl;
      updateDBusSettingsData();
    }
    else
    {
      std::cerr << "[Settings] Failed to update settings!" << std::endl;
    }
  }
  
  // Sync ycbcr420Workaround with detected display ports
  if ( syncOutputPortsSetting() )
  {
    if ( m_settingsManager.writeSettings( m_settings ) )
    {
      std::cout << "[Settings] Synced ycbcr420Workaround settings" << std::endl;
      updateDBusSettingsData();
    }
  }
}

void UccDBusService::serializeProfilesJSON()
{
  std::cout << "[serializeProfilesJSON] Starting profile serialization" << std::endl;
  std::cout << "[serializeProfilesJSON] Default profiles count: " << m_defaultProfiles.size() << std::endl;
  
  // Debug: Check TDP values before serialization
  for ( size_t i = 0; i < m_defaultProfiles.size() && i < 3; ++i )
  {
    std::cout << "[serializeProfilesJSON]   Profile " << i << " (" << m_defaultProfiles[i].id 
              << ") has " << m_defaultProfiles[i].odmPowerLimits.tdpValues.size() << " TDP values" << std::endl;
    if ( !m_defaultProfiles[i].odmPowerLimits.tdpValues.empty() )
    {
      std::cout << "[serializeProfilesJSON]     TDP values: [";
      for ( size_t j = 0; j < m_defaultProfiles[i].odmPowerLimits.tdpValues.size(); ++j )
      {
        if ( j > 0 ) std::cout << ", ";
        std::cout << m_defaultProfiles[i].odmPowerLimits.tdpValues[j];
      }
      std::cout << "]" << std::endl;
    }
  }
  
  const int32_t defaultOnlineCores = getDefaultOnlineCores();
  const int32_t defaultScalingMin = getCpuMinFrequency();
  const int32_t defaultScalingMax = getCpuMaxFrequency();
  
  TccProfile defaultProfile = m_profileManager.getDefaultCustomProfiles()[0];
  
  // serialize all profiles to JSON
  std::ostringstream allProfilesJSON;
  allProfilesJSON << "[";
  
  auto allProfiles = getAllProfiles();
  for ( size_t i = 0; i < allProfiles.size(); ++i )
  {
    if ( i > 0 )
      allProfilesJSON << ",";
    
    allProfilesJSON << profileToJSON( allProfiles[ i ],
                      defaultOnlineCores,
                      defaultScalingMin,
                      defaultScalingMax );
  }
  allProfilesJSON << "]";

  std::ostringstream defaultProfilesJSON;
  defaultProfilesJSON << "[";
  for ( size_t i = 0; i < m_defaultProfiles.size(); ++i )
  {
    if ( i > 0 )
      defaultProfilesJSON << ",";
    
    defaultProfilesJSON << profileToJSON( m_defaultProfiles[ i ],
                        defaultOnlineCores,
                        defaultScalingMin,
                        defaultScalingMax );
  }
  defaultProfilesJSON << "]";

  std::ostringstream customProfilesJSON;
  customProfilesJSON << "[";
  for ( size_t i = 0; i < m_customProfiles.size(); ++i )
  {
    if ( i > 0 )
      customProfilesJSON << ",";
    
    customProfilesJSON << profileToJSON( m_customProfiles[ i ],
                       defaultOnlineCores,
                       defaultScalingMin,
                       defaultScalingMax );
  }
  customProfilesJSON << "]";

  std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
  m_dbusData.profilesJSON = defaultProfilesJSON.str();  // Only default profiles now
  m_dbusData.defaultProfilesJSON = defaultProfilesJSON.str();
  m_dbusData.customProfilesJSON = "[]";  // Empty array since custom profiles are local
  m_dbusData.defaultValuesProfileJSON = profileToJSON( defaultProfile,
                                                       defaultOnlineCores,
                                                       defaultScalingMin,
                                                       defaultScalingMax );
  
  std::cout << "[DBus] Re-serialized profile JSONs" << std::endl;
}

void UccDBusService::fillDeviceSpecificDefaults( std::vector< TccProfile > &profiles )
{
  const int32_t cpuMinFreq = getCpuMinFrequency();
  const int32_t cpuMaxFreq = getCpuMaxFrequency();
  
  // Get TDP info from hardware
  std::vector< TDPInfo > tdpInfo;
  if ( m_odmPowerLimitWorker )
  {
    tdpInfo = m_odmPowerLimitWorker->getTDPInfo();
    std::cout << "[fillDeviceSpecificDefaults] TDP info available: " << tdpInfo.size() << " entries" << std::endl;
    for ( size_t i = 0; i < tdpInfo.size(); ++i )
    {
      std::cout << "[fillDeviceSpecificDefaults]   TDP[" << i << "]: min=" << tdpInfo[i].min 
                << ", max=" << tdpInfo[i].max << ", current=" << tdpInfo[i].current << std::endl;
    }
  }
  else
  {
    std::cout << "[fillDeviceSpecificDefaults] No ODM power limit worker available" << std::endl;
  }
  
  for ( auto &profile : profiles )
  {
    std::cout << "[fillDeviceSpecificDefaults] Filling profile: " << profile.id 
              << ", current TDP values: " << profile.odmPowerLimits.tdpValues.size() << std::endl;
    
    // Fill CPU frequency defaults
    if ( !profile.cpu.scalingMinFrequency.has_value() || profile.cpu.scalingMinFrequency.value() < cpuMinFreq )
    {
      profile.cpu.scalingMinFrequency = cpuMinFreq;
    }
    
    if ( !profile.cpu.scalingMaxFrequency.has_value() )
    {
      profile.cpu.scalingMaxFrequency = cpuMaxFreq;
    }
    else if ( profile.cpu.scalingMaxFrequency.value() < profile.cpu.scalingMinFrequency.value() )
    {
      profile.cpu.scalingMaxFrequency = profile.cpu.scalingMinFrequency;
    }
    else if ( profile.cpu.scalingMaxFrequency.value() > cpuMaxFreq )
    {
      profile.cpu.scalingMaxFrequency = cpuMaxFreq;
    }
    
    // Fill TDP values if missing and hardware TDP info is available
    if ( !tdpInfo.empty() && tdpInfo.size() > profile.odmPowerLimits.tdpValues.size() )
    {
      const size_t nrMissingValues = tdpInfo.size() - profile.odmPowerLimits.tdpValues.size();
      std::cout << "[fillDeviceSpecificDefaults]   Adding " << nrMissingValues << " TDP values" << std::endl;
      // Add missing TDP values with max values from hardware
      for ( size_t i = profile.odmPowerLimits.tdpValues.size(); i < tdpInfo.size(); ++i )
      {
        profile.odmPowerLimits.tdpValues.push_back( tdpInfo[i].max );
        std::cout << "[fillDeviceSpecificDefaults]     Added TDP[" << i << "] = " << tdpInfo[i].max << std::endl;
      }
    }
    
    std::cout << "[fillDeviceSpecificDefaults]   Final TDP values: " << profile.odmPowerLimits.tdpValues.size() << std::endl;
  }
}

void UccDBusService::loadAutosave()
{
  m_autosave = m_autosaveManager.readAutosave();
  std::cout << "[Autosave] Loaded autosave (displayBrightness: " 
            << m_autosave.displayBrightness << "%)" << std::endl;
}

void UccDBusService::saveAutosave()
{
  if ( m_autosaveManager.writeAutosave( m_autosave ) )
  {
    std::cout << "[Autosave] Saved autosave" << std::endl;
  }
  else
  {
    std::cerr << "[Autosave] Failed to save autosave!" << std::endl;
  }
}

std::vector< std::vector< std::string > > UccDBusService::getOutputPorts()
{
  std::vector< std::vector< std::string > > result;
  
  struct udev *udev_context = udev_new();
  if ( !udev_context )
  {
    std::cerr << "[OutputPorts] Failed to create udev context" << std::endl;
    return result;
  }
  
  struct udev_enumerate *drm_devices = udev_enumerate_new( udev_context );
  if ( !drm_devices )
  {
    std::cerr << "[OutputPorts] Failed to enumerate devices" << std::endl;
    udev_unref( udev_context );
    return result;
  }
  
  if ( udev_enumerate_add_match_subsystem( drm_devices, "drm" ) < 0 ||
       udev_enumerate_add_match_sysname( drm_devices, "card*-*-*" ) < 0 ||
       udev_enumerate_scan_devices( drm_devices ) < 0 )
  {
    std::cerr << "[OutputPorts] Failed to scan devices" << std::endl;
    udev_enumerate_unref( drm_devices );
    udev_unref( udev_context );
    return result;
  }
  
  struct udev_list_entry *drm_devices_iterator = udev_enumerate_get_list_entry( drm_devices );
  if ( !drm_devices_iterator )
  {
    udev_enumerate_unref( drm_devices );
    udev_unref( udev_context );
    return result;
  }
  
  struct udev_list_entry *drm_devices_entry;
  udev_list_entry_foreach( drm_devices_entry, drm_devices_iterator )
  {
    std::string path = udev_list_entry_get_name( drm_devices_entry );
    std::string name = path.substr( path.rfind( "/" ) + 1 );
    
    // Extract card number (e.g., "card0" -> 0)
    size_t cardPos = name.find( "card" );
    size_t dashPos = name.find( "-", cardPos );
    if ( cardPos == std::string::npos || dashPos == std::string::npos )
      continue;
    
    int cardNumber = std::stoi( name.substr( cardPos + 4, dashPos - cardPos - 4 ) );
    
    // Ensure result vector is large enough
    if ( static_cast< size_t >( cardNumber + 1 ) > result.size() )
    {
      result.resize( cardNumber + 1 );
    }
    
    // Extract port name (everything after "card0-")
    std::string portName = name.substr( dashPos + 1 );
    result[cardNumber].push_back( portName );
  }
  
  udev_enumerate_unref( drm_devices );
  udev_unref( udev_context );
  
  return result;
}

bool UccDBusService::syncOutputPortsSetting()
{
  bool settingsChanged = false;
  
  auto outputPorts = getOutputPorts();
  
  // Delete additional cards from settings
  if ( m_settings.ycbcr420Workaround.size() > outputPorts.size() )
  {
    m_settings.ycbcr420Workaround.resize( outputPorts.size() );
    settingsChanged = true;
  }
  
  for ( size_t card = 0; card < outputPorts.size(); ++card )
  {
    // Add card to settings if missing
    if ( m_settings.ycbcr420Workaround.size() <= card )
    {
      YCbCr420Card newCard;
      newCard.card = static_cast< int >( card );
      m_settings.ycbcr420Workaround.push_back( newCard );
      settingsChanged = true;
    }
    
    // Get reference to card settings
    auto &cardSettings = m_settings.ycbcr420Workaround[card];
    
    // Delete ports that no longer exist
    std::vector< std::string > portsToRemove;
    
    for ( const auto &portEntry : cardSettings.ports )
    {
      bool stillAvailable = false;
      for ( const auto &port : outputPorts[card] )
      {
        if ( portEntry.port == port )
        {
          stillAvailable = true;
          break;
        }
      }
      
      if ( !stillAvailable )
      {
        portsToRemove.push_back( portEntry.port );
      }
    }
    
    // Remove ports that are no longer available
    for ( const auto &port : portsToRemove )
    {
      cardSettings.ports.erase(
        std::remove_if( cardSettings.ports.begin(), cardSettings.ports.end(),
                       [&port]( const YCbCr420Port &p ) { return p.port == port; } ),
        cardSettings.ports.end()
      );
      settingsChanged = true;
    }
    
    // Add missing ports to settings
    for ( const auto &port : outputPorts[card] )
    {
      bool found = false;
      for ( const auto &portEntry : cardSettings.ports )
      {
        if ( portEntry.port == port )
        {
          found = true;
          break;
        }
      }
      
      if ( !found )
      {
        YCbCr420Port newPort;
        newPort.port = port;
        newPort.enabled = false;
        cardSettings.ports.push_back( newPort );
        settingsChanged = true;
      }
    }
  }
  
  return settingsChanged;
}
