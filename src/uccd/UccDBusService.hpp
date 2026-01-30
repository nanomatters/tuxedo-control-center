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

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <optional>
#include "workers/DaemonWorker.hpp"
#include "workers/GpuInfoWorker.hpp"
#include "workers/WebcamWorker.hpp"
#include "workers/DisplayBacklightWorker.hpp"
#include "workers/CpuWorker.hpp"
#include "workers/ODMPowerLimitWorker.hpp"
#include "workers/CpuPowerWorker.hpp"
#include "workers/FanControlWorker.hpp"
#include "workers/YCbCr420WorkaroundWorker.hpp"
#include "workers/NVIDIAPowerCTRLListener.hpp"
#include "workers/KeyboardBacklightListener.hpp"
#include "workers/ChargingWorker.hpp"
#include "workers/DisplayRefreshRateWorker.hpp"
#include "workers/PrimeWorker.hpp"
#include "workers/ODMProfileWorker.hpp"
#include "FnLockController.hpp"
#include "profiles/UccProfile.hpp"
#include "profiles/DefaultProfiles.hpp"
#include "ProfileManager.hpp"
#include "SettingsManager.hpp"
#include "AutosaveManager.hpp"
#include "TccSettings.hpp"
#include "tuxedo_io_lib/tuxedo_io_api.hh"

// Forward declarations
class GpuInfoWorker;
class UccDBusService;

// helper functions for JSON serialization
std::string dgpuInfoToJSON( const DGpuInfo &info );
std::string igpuInfoToJSON( const IGpuInfo &info );

/**
 * @brief Time-stamped data structure
 *
 * Holds a value along with its timestamp for DBus transmission.
 */
template< typename T >
struct TimeData
{
  int64_t timestamp;
  T data;

  TimeData() : timestamp( 0 ), data{} {}

  TimeData( int64_t ts, const T &value ) : timestamp( ts ), data( value ) {}

  void set( int64_t ts, const T &value )
  {
    timestamp = ts;
    data = value;
  }
};

/**
 * @brief Fan data structure
 *
 * Contains timestamped speed and temperature data for a fan.
 */
struct FanData
{
  TimeData< int32_t > speed;
  TimeData< int32_t > temp;

  FanData() : speed(), temp() {}
};

/**
 * @brief DBus data container
 *
 * Contains all data that is exposed via the DBus interface.
 * This structure mirrors the UccDBusData TypeScript class.
 */
class UccDBusData
{
public:
  std::string device;
  std::string displayModes;
  bool isX11;
  bool tuxedoWmiAvailable;
  bool fanHwmonAvailable;
  std::string tccdVersion;
  std::vector< FanData > fans;
  bool webcamSwitchAvailable;
  bool webcamSwitchStatus;
  bool forceYUV420OutputSwitchAvailable;
  std::string dGpuInfoValuesJSON;
  std::string iGpuInfoValuesJSON;
  std::string cpuPowerValuesJSON;
  std::string primeState;
  bool modeReapplyPending;
  std::string tempProfileName;
  std::string tempProfileId;
  std::string activeProfileJSON;
  std::string profilesJSON;
  std::string customProfilesJSON;
  std::string defaultProfilesJSON;
  std::string defaultValuesProfileJSON;
  std::string settingsJSON;
  std::vector< std::string > odmProfilesAvailable;
  std::string odmPowerLimitsJSON;
  std::string keyboardBacklightCapabilitiesJSON;
  std::string keyboardBacklightStatesJSON;
  std::string keyboardBacklightStatesNewJSON;
  int32_t fansMinSpeed;
  bool fansOffAvailable;
  std::string chargingProfilesAvailable;
  std::string currentChargingProfile;
  std::string chargingPrioritiesAvailable;
  std::string currentChargingPriority;
  std::string chargeStartAvailableThresholds;
  std::string chargeEndAvailableThresholds;
  int32_t chargeStartThreshold;
  int32_t chargeEndThreshold;
  std::string chargeType;
  bool fnLockSupported;
  bool fnLockStatus;
  bool sensorDataCollectionStatus;
  bool d0MetricsUsage;
  int32_t nvidiaPowerCTRLDefaultPowerLimit;
  int32_t nvidiaPowerCTRLMaxPowerLimit;
  bool nvidiaPowerCTRLAvailable;

  std::mutex dataMutex;

  explicit UccDBusData( int numberFans = 3 )
    : device( "unknown" ),
      displayModes( "[]" ),
      isX11( false ),
      tuxedoWmiAvailable( false ),
      fanHwmonAvailable( false ),
      tccdVersion( "0.0.0" ),
      fans( numberFans ),
      webcamSwitchAvailable( false ),
      webcamSwitchStatus( false ),
      forceYUV420OutputSwitchAvailable( false ),
      dGpuInfoValuesJSON( "{}" ),
      iGpuInfoValuesJSON( "{}" ),
      cpuPowerValuesJSON( "{}" ),
      primeState( "unknown" ),
      modeReapplyPending( false ),
      tempProfileName( "" ),
      tempProfileId( "" ),
      activeProfileJSON( "{}" ),
      profilesJSON( "[]" ),
      customProfilesJSON( "[]" ),
      defaultProfilesJSON( "[]" ),
      defaultValuesProfileJSON( "{}" ),
      settingsJSON( "{}" ),
      odmProfilesAvailable(),
      odmPowerLimitsJSON( "{}" ),
      keyboardBacklightCapabilitiesJSON( "{}" ),
      keyboardBacklightStatesJSON( "{}" ),
      keyboardBacklightStatesNewJSON( "{}" ),
      fansMinSpeed( 0 ),
      fansOffAvailable( false ),
      chargingProfilesAvailable( "[]" ),
      currentChargingProfile( "" ),
      chargingPrioritiesAvailable( "[]" ),
      currentChargingPriority( "" ),
      chargeStartAvailableThresholds( "[]" ),
      chargeEndAvailableThresholds( "[]" ),
      chargeStartThreshold( -1 ),
      chargeEndThreshold( -1 ),
      chargeType( "Unknown" ),
      fnLockSupported( false ),
      fnLockStatus( false ),
      sensorDataCollectionStatus( false ),
      d0MetricsUsage( false ),
      nvidiaPowerCTRLDefaultPowerLimit( 0 ),
      nvidiaPowerCTRLMaxPowerLimit( 1000 ),
      nvidiaPowerCTRLAvailable( false )
  {
  }
};

/**
 * @brief TCC DBus Interface Adaptor
 *
 * Implements the com.uniwill.uccd DBus interface.
 * Handles all method calls from DBus clients and provides access to daemon data.
 */
class UccDBusInterfaceAdaptor
{
public:
  static constexpr const char* INTERFACE_NAME = "com.tuxedocomputers.tccd";

  /**
   * @brief Constructor
   * @param object DBus object interface
   * @param data Shared data structure (includes mutex)
   * @param service Reference to UccDBusService for profile operations
   */
  explicit UccDBusInterfaceAdaptor( sdbus::IObject &object,
                                    UccDBusData &data,
                                    UccDBusService *service = nullptr );

  ~UccDBusInterfaceAdaptor() = default;

  // device and system information
  std::string GetDeviceName();
  std::string GetDisplayModesJSON();
  bool GetIsX11();
  bool TuxedoWmiAvailable();
  bool FanHwmonAvailable();
  std::string TccdVersion();

  // fan data methods
  std::map< std::string, std::map< std::string, sdbus::Variant > > GetFanDataCPU();
  std::map< std::string, std::map< std::string, sdbus::Variant > > GetFanDataGPU1();
  std::map< std::string, std::map< std::string, sdbus::Variant > > GetFanDataGPU2();

  // webcam and display methods
  bool WebcamSWAvailable();
  bool GetWebcamSWStatus();
  bool GetForceYUV420OutputSwitchAvailable();
  bool SetDisplayRefreshRate( const std::string &display, int refreshRate );

  // gpu information methods
  std::string GetDGpuInfoValuesJSON();
  std::string GetIGpuInfoValuesJSON();
  std::string GetCpuPowerValuesJSON();

  // graphics methods
  std::string GetPrimeState();
  bool ConsumeModeReapplyPending();

  // profile methods
  std::string GetActiveProfileJSON();
  std::string GetPowerState();
  bool SetTempProfile( const std::string &profileName );
  bool SetTempProfileById( const std::string &id );
  bool SetActiveProfile( const std::string &id );
  bool ApplyProfile( const std::string &profileJSON );
  std::string GetProfilesJSON();
  std::string GetCustomProfilesJSON();
  // Fan profile get/set for editable (custom) profiles only
  bool SetFanProfileCPU( const std::string &pointsJSON );
  bool SetFanProfileDGPU( const std::string &pointsJSON );
  bool ApplyFanProfiles( const std::string &fanProfilesJSON );
  bool RevertFanProfiles();
  std::string GetDefaultProfilesJSON();
  std::string GetDefaultValuesProfileJSON();
  bool AddCustomProfile( const std::string &profileJSON );
  bool SaveCustomProfile( const std::string &profileJSON );
  bool DeleteCustomProfile( const std::string &profileId );
  bool UpdateCustomProfile( const std::string &profileJSON );
  std::string CopyProfile( const std::string &sourceId, const std::string &newName );
  std::string GetFanProfile( const std::string &name );
  std::string GetFanProfileNames();
  bool SetFanProfile( const std::string &name, const std::string &json );

  // settings methods
  std::string GetSettingsJSON();
  bool SetStateMap( const std::string &state, const std::string &profileId );

  // odm methods
  std::vector< std::string > ODMProfilesAvailable();
  std::string ODMPowerLimitsJSON();

  // keyboard backlight methods
  std::string GetKeyboardBacklightCapabilitiesJSON();
  std::string GetKeyboardBacklightStatesJSON();
  bool SetKeyboardBacklightStatesJSON( const std::string &keyboardBacklightStatesJSON );

  // fan control methods
  int32_t GetFansMinSpeed();
  bool GetFansOffAvailable();

  // charging methods
  std::string GetChargingProfilesAvailable();
  std::string GetCurrentChargingProfile();
  bool SetChargingProfile( const std::string &profileDescriptor );
  std::string GetChargingPrioritiesAvailable();
  std::string GetCurrentChargingPriority();
  bool SetChargingPriority( const std::string &priorityDescriptor );
  std::string GetChargeStartAvailableThresholds();
  std::string GetChargeEndAvailableThresholds();
  int32_t GetChargeStartThreshold();
  int32_t GetChargeEndThreshold();
  bool SetChargeStartThreshold( int32_t value );
  bool SetChargeEndThreshold( int32_t value );
  std::string GetChargeType();
  bool SetChargeType( const std::string &type );

  // fn lock methods
  bool GetFnLockSupported();
  bool GetFnLockStatus();
  void SetFnLockStatus( bool status );

  // sensor data collection methods
  void SetSensorDataCollectionStatus( bool status );
  bool GetSensorDataCollectionStatus();
  void SetDGpuD0Metrics( bool status );

  // nvidia power control methods
  int32_t GetNVIDIAPowerCTRLDefaultPowerLimit();
  int32_t GetNVIDIAPowerCTRLMaxPowerLimit();
  bool GetNVIDIAPowerCTRLAvailable();

  // signal emitters
  void emitModeReapplyPendingChanged( bool pending );
  void emitProfileChanged( const std::string &profileId );

  // allow UccDBusService to access timeout handling
  friend class UccDBusService;

  // Required for manual vtable registration in sdbus-c++ 2.x
  void registerAdaptor();

private:
  sdbus::IObject &m_object;
  UccDBusData &m_data;
  UccDBusService *m_service;
  std::chrono::steady_clock::time_point m_lastDataCollectionAccess;

  void resetDataCollectionTimeout();
  std::map< std::string, std::map< std::string, sdbus::Variant > > exportFanData( const FanData &fanData );
};

/**
 * @brief TCC DBus Service Worker
 *
 * Manages the DBus service lifecycle as a daemon worker.
 * Exports the TCC interface on the system bus and handles periodic updates.
 */
class UccDBusService : public DaemonWorker
{
public:
  /**
   * @brief Constructor
   */
  UccDBusService();

  virtual ~UccDBusService() = default;

  // Prevent copy and move
  UccDBusService( const UccDBusService & ) = delete;
  UccDBusService( UccDBusService && ) = delete;
  UccDBusService &operator=( const UccDBusService & ) = delete;
  UccDBusService &operator=( UccDBusService && ) = delete;

  /**
   * @brief Get the DBus interface adaptor
   * @return Pointer to adaptor or nullptr if not initialized
   */
  UccDBusInterfaceAdaptor *getAdaptor() noexcept
  {
    return m_adaptor.get();
  }

  /**
   * @brief Get the DBus data reference for testing
   */
  const UccDBusData &getDbusData() const noexcept
  {
    return m_dbusData;
  }

  // profile management methods
  UccProfile getCurrentProfile() const;
  bool setCurrentProfileByName( const std::string &profileName );
  bool setCurrentProfileById( const std::string &id );
  bool applyProfileJSON( const std::string &profileJSON );
  std::vector< UccProfile > getAllProfiles() const;
  std::vector< UccProfile > getDefaultProfiles() const;
  std::vector< UccProfile > getCustomProfiles() const;
  UccProfile getDefaultProfile() const;
  void updateDBusActiveProfileData();
  void updateDBusSettingsData();
  
  // profile manipulation methods
  bool addCustomProfile( const UccProfile &profile );
  bool deleteCustomProfile( const std::string &profileId );
  bool updateCustomProfile( const UccProfile &profile );
  std::optional< UccProfile > copyProfile( const std::string &sourceId, const std::string &newName );

  // Allow UccDBusInterfaceAdaptor to access private members
  friend class UccDBusInterfaceAdaptor;

protected:
  void onStart() override;
  void onWork() override;
  void onExit() override;

private:
  static constexpr const char* INTERFACE_NAME = "com.tuxedocomputers.tccd";
  UccDBusData m_dbusData;
  TuxedoIOAPI m_io;
  std::unique_ptr< sdbus::IConnection > m_connection;
  std::unique_ptr< sdbus::IObject > m_object;
  std::unique_ptr< UccDBusInterfaceAdaptor > m_adaptor;
  bool m_started;

  // profile management
  ProfileManager m_profileManager;
  SettingsManager m_settingsManager;
  AutosaveManager m_autosaveManager;
  TccSettings m_settings;
  TccAutosave m_autosave;
  UccProfile m_activeProfile;
  std::vector< UccProfile > m_defaultProfiles;
  std::vector< UccProfile > m_customProfiles;
  
  // state switching
  ProfileState m_currentState;
  std::string m_currentStateProfileId;

  void setupGpuDataCallback();
  void updateFanData();
  void loadProfiles();
  void loadSettings();
  void loadAutosave();
  void saveAutosave();
  void initializeProfiles();
  void initializeDisplayModes();
  void serializeProfilesJSON();
  void fillDeviceSpecificDefaults( std::vector< UccProfile > &profiles );
  std::optional< TUXEDODevice > identifyDevice();
  bool syncOutputPortsSetting();
  std::vector< std::vector< std::string > > getOutputPorts();

  // workers
  GpuInfoWorker m_gpuWorker;
  WebcamWorker m_webcamWorker;
  std::unique_ptr< DisplayBacklightWorker > m_displayBacklightWorker;
  std::unique_ptr< CpuWorker > m_cpuWorker;
  std::unique_ptr< ODMPowerLimitWorker > m_odmPowerLimitWorker;
  std::unique_ptr< CpuPowerWorker > m_cpuPowerWorker;
  std::unique_ptr< FanControlWorker > m_fanControlWorker;
  std::unique_ptr< YCbCr420WorkaroundWorker > m_ycbcr420Worker;
  std::unique_ptr< NVIDIAPowerCTRLListener > m_nvidiaPowerListener;
  std::unique_ptr< KeyboardBacklightListener > m_keyboardBacklightListener;
  std::unique_ptr< ChargingWorker > m_chargingWorker;
  std::unique_ptr< DisplayRefreshRateWorker > m_displayRefreshRateWorker;
  std::unique_ptr< PrimeWorker > m_primeWorker;
  std::unique_ptr< ODMProfileWorker > m_odmProfileWorker;

  // controllers
  FnLockController m_fnLockController;

  static constexpr const char *SERVICE_NAME = "com.tuxedocomputers.tccd";
  static constexpr const char *OBJECT_PATH = "/com/uniwill/uccd";
};
