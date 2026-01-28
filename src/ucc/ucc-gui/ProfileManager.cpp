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

#include "ProfileManager.hpp"
#include <QDebug>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <QFile>
#include <QDir>

namespace ucc
{

ProfileManager::ProfileManager( QObject *parent )
  : QObject( parent )
  , m_client( std::make_unique< TccdClient >( this ) )
  , m_settings( std::make_unique< QSettings >( QDir::homePath() + "/.config/uccrc", QSettings::IniFormat, this ) )
{
  FILE *log = fopen( "/tmp/ucc-debug.log", "a" );
  fprintf( log, "ProfileManager constructor called\n" );
  fprintf( log, "QSettings file path: %s\n", m_settings->fileName().toStdString().c_str() );
  fflush( log );
  
  m_connected = m_client->isConnected();
  fprintf( log, "Connected status: %d\n", m_connected );
  fflush( log );

  // Load local custom fan profiles regardless of DBus connection
  loadCustomFanProfilesFromSettings();

  if ( m_connected )
  {
    // Fetch hardware power limits immediately
    m_hardwarePowerLimits = m_client->getODMPowerLimits().value_or( std::vector< int >() );
    fprintf( log, "ProfileManager constructor: Fetched hardware limits, size: %zu\n", m_hardwarePowerLimits.size() );
    fflush( log );
    
    // Connect to profile changed signal
    connect( m_client.get(), &TccdClient::profileChanged,
             this, [this]( const QString &profileId ) {
      onProfileChanged( profileId.toStdString() );
    } );

    // Connect to power state changed signal
    connect( m_client.get(), &TccdClient::powerStateChanged,
             this, [this]( const QString &state ) {
      onPowerStateChanged( state );
    } );

    // Load custom profiles from local storage
    loadCustomProfilesFromSettings();
    
    // No need to send profiles to tccd-ng; UCC handles profile application
    
    // No need to set stateMap in tccd-ng; UCC handles power state changes
    
    // DO NOT load profiles here - defer to after signals are connected
    fprintf( log, "NOT calling updateProfiles() in constructor - will be called later\n" );
    fflush( log );
  }
  else
  {
    fprintf( log, "WARNING: Not connected to DBus!\n" );
    fflush( log );
  }

  fprintf( log, "ProfileManager constructor done\n" );
  fflush( log );
  fclose( log );
  
  emit connectedChanged();

  // initialize sameSpeed property from daemon if available
  if ( m_client && m_client->isConnected() )
  {
    try {
      auto val = m_client->getFanSameSpeed();
      if ( val.has_value() )
      {
        emit sameSpeedChanged();
      }
    } catch ( const std::exception & ) { /* ignore */ }
  }

  // Connect profile change signal to emit sameSpeedChanged so QML updates
  connect( m_client.get(), &TccdClient::profileChanged,
           this, [this]( const QString & ) { emit sameSpeedChanged(); } );

  // Implement sameSpeed accessors via TccdClient
  // (declared in header)
}

void ProfileManager::refresh()
{
  updateProfiles();
}

bool ProfileManager::sameSpeed() const
{
  if ( m_client )
  {
    try {
      auto val = m_client->getFanSameSpeed();
      if ( val.has_value() ) return *val;
    } catch ( const std::exception & ) { }
  }
  return true; // default
}

bool ProfileManager::setSameSpeed( bool same )
{
  if ( !m_client ) return false;
  try {
    if ( m_client->setFanSameSpeed( same ) )
    {
      emit sameSpeedChanged();
      return true;
    }
  } catch ( const std::exception & ) { }
  return false;
}

void ProfileManager::updateProfiles()
{
  FILE *log = fopen( "/tmp/ucc-debug.log", "a" );
  fprintf( log, "updateProfiles() called\n" );
  fflush( log );
  
  // Fetch default profiles if not already loaded
  if (m_defaultProfilesData.isEmpty()) {
    try {
      if ( auto json = m_client->getDefaultProfilesJSON() )
      {
        QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( *json ).toUtf8() );
        if ( doc.isArray() )
        {
          m_defaultProfilesData = doc.array();
          m_defaultProfiles.clear();
          for ( const auto &profile : m_defaultProfilesData )
          {
            if ( profile.isObject() )
            {
              QString name = profile.toObject()["name"].toString();
              if ( !name.isEmpty() )
              {
                m_defaultProfiles.append( name );
              }
            }
          }
        }
      }
    } catch (const std::exception &e) {
      qWarning() << "Failed to get default profiles:" << e.what();
    }
  }
  
  // Default profiles are cached
  emit defaultProfilesChanged();
  emit sameSpeedChanged();
  emit sameSpeedChanged();

  // Custom profiles are loaded from local storage, no need to fetch from server
  emit customProfilesChanged();
  emit sameSpeedChanged();
  emit sameSpeedChanged();

  // Ensure combined list is up-to-date before deciding which profile to activate
  updateAllProfiles();

  // Do not persist an 'activeProfile' in settings anymore.
  // Decide which profile to activate based on the current power state and stateMap.
  // Ask the daemon for the current power state if we don't yet have it.
  if ( m_powerState.isEmpty() && m_client ) {
    try {
      if ( auto ps = m_client->getPowerState() ) {
        m_powerState = QString::fromStdString( *ps );
        emit powerStateChanged();
      }
    } catch ( const std::exception &e ) {
      qWarning() << "Failed to query daemon power state:" << e.what();
    }
  }

  // Refresh sameSpeed state
  try {
    emit sameSpeedChanged();
  } catch ( const std::exception & ) { }


  QString desiredProfile;
  if ( !m_powerState.isEmpty() ) {
    desiredProfile = resolveStateMapToProfileName( m_powerState );
  } else {
    // If we still don't know power state, default to AC mapping if available
    desiredProfile = resolveStateMapToProfileName( "power_ac" );
  }

  if ( !desiredProfile.isEmpty() && m_allProfiles.contains( desiredProfile ) && desiredProfile != m_activeProfile )
  {
    fprintf( log, "Applying desired profile based on state: %s\n", desiredProfile.toStdString().c_str() );
    fflush( log );
    setActiveProfile( desiredProfile );
  }

  // Update combined list and index
  fprintf( log, "Calling updateAllProfiles()\n" );
  fflush( log );
  updateAllProfiles();
  updateActiveProfileIndex();
  fprintf( log, "updateProfiles() done\n" );
  fflush( log );
  fclose( log );

  // Emit sameSpeedChanged so QML can refresh checkbox when profile changes
  try {
    if ( m_client && m_client->isConnected() ) { emit sameSpeedChanged(); }
  } catch ( const std::exception & ) { }
}


void ProfileManager::setActiveProfile( const QString &profileId )
{
  // profileId might actually be a profile name (from QML modelData)
  // Try to find the actual ID from the profile name
  QString actualId = profileId;
  
  // Search in default profiles
  for ( const auto &profile : m_defaultProfilesData )
  {

    if ( profile.isObject() )
    {
      QJsonObject obj = profile.toObject();

      if ( obj["name"].toString() == profileId )
      {
        actualId = obj["id"].toString();
        break;
      }
    }
  }
  
  // Search in custom profiles if not found

  if ( actualId == profileId )
  {
    for ( const auto &profile : m_customProfilesData )
    {

      if ( profile.isObject() )
      {
        QJsonObject obj = profile.toObject();

        if ( obj["name"].toString() == profileId )
        {
          actualId = obj["id"].toString();
          break;
        }
      }
    }
  }
  
  // Check if this is a custom profile
  bool isCustom = false;
  QString profileData;
  for (const auto &profile : m_customProfilesData) {
    QJsonObject obj = profile.toObject();
    if (obj.value("id").toString() == actualId) {
      isCustom = true;
      profileData = QJsonDocument(obj).toJson(QJsonDocument::Compact);
      break;
    }
  }
  
  bool success = false;
  if (isCustom && !profileData.isEmpty()) {
    // Send full profile data for custom profiles
    try {
      success = m_client->applyProfile(profileData.toStdString());
    } catch (const std::exception &e) {
      qWarning() << "Failed to apply custom profile:" << e.what();
    }
    qDebug() << "Custom profile applied:" << profileId << "(ID:" << actualId << ")";
  } else {
    // Use ID for default profiles
    try {
      success = m_client->setActiveProfile(actualId.toStdString());
    } catch (const std::exception &e) {
      qWarning() << "Failed to set active profile:" << e.what();
    }
    qDebug() << "Default profile activated:" << profileId << "(ID:" << actualId << ")";
  }
  
  // Always update local state and emit signals, regardless of DBus success
  // The UI should reflect the selected profile even if application to system fails
  if (m_activeProfile != profileId) {
    m_activeProfile = profileId;
    emit activeProfileChanged();
  }
  updateAllProfiles();
  updateActiveProfileIndex();
  
  if (!success) {
    emit error("Failed to activate profile on system: " + profileId);
  }
}

void ProfileManager::saveProfile( const QString &profileJSON )
{
  QJsonDocument doc = QJsonDocument::fromJson(profileJSON.toUtf8());
  if (!doc.isObject()) {
    emit error("Invalid profile JSON");
    return;
  }
  
  QJsonObject profileObj = doc.object();
  QString profileId = profileObj.value("id").toString();
  QString profileName = profileObj.value("name").toString();
  
  if (profileId.isEmpty() || profileName.isEmpty()) {
    emit error("Profile missing id or name");
    return;
  }
  
  // Check if profile already exists and remember old name
  int foundIndex = -1;
  QString oldName;
  for (int i = 0; i < m_customProfilesData.size(); ++i) {
    QJsonObject existingProfile = m_customProfilesData[i].toObject();
    if (existingProfile.value("id").toString() == profileId) {
      // remember index and old name before replacing
      foundIndex = i;
      oldName = existingProfile.value("name").toString();
      break;
    }
  }
  
  if (foundIndex == -1) {
    // Add new profile
    m_customProfilesData.append(profileObj);
    m_customProfiles.append(profileName);
  } else {
    // Update existing profile object
    m_customProfilesData[foundIndex] = profileObj;

    // If the name changed, update the names list so UI widgets refresh
    if (!oldName.isEmpty() && oldName != profileName) {
      int nameIndex = m_customProfiles.indexOf(oldName);
      if (nameIndex != -1) {
        m_customProfiles.replace(nameIndex, profileName);
      } else {
        // Fallback: ensure the new name is present
        if (!m_customProfiles.contains(profileName))
          m_customProfiles.append(profileName);
      }
    }
  }
  
  // Save to local storage
  saveCustomProfilesToSettings();
  
  // Update the UI
  updateAllProfiles();
  
  qDebug() << "Profile saved locally:" << profileName;
}

void ProfileManager::deleteProfile( const QString &profileId )
{
  // Find and remove the profile
  for (int i = 0; i < m_customProfilesData.size(); ++i) {
    QJsonObject profileObj = m_customProfilesData[i].toObject();
    if (profileObj.value("id").toString() == profileId) {
      QString profileName = profileObj.value("name").toString();
      m_customProfilesData.removeAt(i);
      m_customProfiles.removeAll(profileName);
      
      // Save to local storage
      saveCustomProfilesToSettings();
      
      // Update the UI
      updateAllProfiles();
      
      qDebug() << "Profile deleted locally:" << profileName;
      return;
    }
  }
  
  emit error("Profile not found: " + profileId);
}

QString ProfileManager::createProfileFromDefault( const QString &name )
{
  // Get default profile template from server
  if (auto defaultJson = m_client->getDefaultValuesProfileJSON()) {
    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(*defaultJson).toUtf8());
    if (doc.isObject()) {
      QJsonObject profileObj = doc.object();
      
      // Generate a unique ID
      QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
      
      // Set the name and ID
      profileObj["name"] = name;
      profileObj["id"] = id;
      
      // Add to custom profiles
      m_customProfilesData.append(profileObj);
      m_customProfiles.append(name);
      
      // Save to local storage
      saveCustomProfilesToSettings();
      
      // Update the UI
      updateAllProfiles();
      
      qDebug() << "Created new profile from default:" << name;
      
      // Return the profile JSON
      return QJsonDocument(profileObj).toJson(QJsonDocument::Compact);
    }
  }
  
  emit error("Failed to get default profile template");
  return QString();
}

QString ProfileManager::getProfileDetails( const QString &profileId )
{
  // Search in default profiles
  for ( const auto &profile : m_defaultProfilesData )
  {
    if ( profile.toObject()["id"].toString() == profileId )
    {
      return QJsonDocument( profile.toObject() ).toJson( QJsonDocument::Compact );
    }
  }

  // Search in custom profiles
  for ( const auto &profile : m_customProfilesData )
  {
    if ( profile.toObject()["id"].toString() == profileId )
    {
      return QJsonDocument( profile.toObject() ).toJson( QJsonDocument::Compact );
    }
  }

  return QString();
}

QString ProfileManager::getProfileIdByName( const QString &profileName )
{
  // Search in default profiles
  for ( const auto &profile : m_defaultProfilesData )
  {
    if ( profile.toObject()["name"].toString() == profileName )
    {
      return profile.toObject()["id"].toString();
    }
  }

  // Search in custom profiles
  for ( const auto &profile : m_customProfilesData )
  {
    if ( profile.toObject()["name"].toString() == profileName )
    {
      return profile.toObject()["id"].toString();
    }
  }

  return QString();
}

void ProfileManager::onProfileChanged( const std::string &profileId )
{
  qDebug() << "Profile changed signal:" << QString::fromStdString( profileId );
  updateProfiles();
}

void ProfileManager::onPowerStateChanged( const QString &state )
{
  qDebug() << "Power state changed:" << state;

  // Update internal power state and notify GUI
  m_powerState = state;
  emit powerStateChanged();

  // Resolve mapped profile and apply it
  QString desiredProfile = resolveStateMapToProfileName( state );
  if ( desiredProfile.isEmpty() )
  {
    qWarning() << "No profile mapped for state:" << state;
    return;
  }

  qDebug() << "Applying profile" << desiredProfile << "for power state" << state;
  QString profileJson = getProfileDetails( desiredProfile );
  if ( profileJson.isEmpty() )
  {
    qWarning() << "Profile not found for:" << desiredProfile;
    return;
  }

  try {
    m_client->applyProfile( profileJson.toStdString() );
  } catch (const std::exception &e) {
    qWarning() << "Failed to apply profile:" << e.what();
    return;
  }

  // Update active profile name and notify
  if ( m_activeProfile != desiredProfile ) {
    m_activeProfile = desiredProfile;
    emit activeProfileChanged();
    updateAllProfiles();
    updateActiveProfileIndex();
  }
}

void ProfileManager::updateAllProfiles()
{
  FILE *log = fopen( "/tmp/ucc-debug.log", "a" );
  fprintf( log, "updateAllProfiles() called\n" );
  fprintf( log, "  m_defaultProfiles count: %ld\n", (long)m_defaultProfiles.size() );
  fprintf( log, "  m_customProfiles count: %ld\n", (long)m_customProfiles.size() );
  fprintf( log, "  m_allProfiles count (before): %ld\n", (long)m_allProfiles.size() );
  fflush( log );
  
  QStringList newAllProfiles;
  newAllProfiles.append( m_defaultProfiles );
  newAllProfiles.append( m_customProfiles );

  fprintf( log, "  newAllProfiles count: %ld\n", (long)newAllProfiles.size() );
  for ( int i = 0; i < newAllProfiles.size(); ++i )
  {
    fprintf( log, "    Profile %d: %s\n", i, newAllProfiles[i].toStdString().c_str() );
  }
  fflush( log );

  if ( m_allProfiles != newAllProfiles )
  {
    fprintf( log, "  Profiles changed! Emitting signal.\n" );
    fflush( log );
    m_allProfiles = newAllProfiles;
    fprintf( log, "  About to emit allProfilesChanged()\n" );
    fflush( log );
    emit allProfilesChanged();
    fprintf( log, "  allProfilesChanged() emitted\n" );
    fflush( log );
  }
  else
  {
    fprintf( log, "  Profiles did NOT change. No signal emitted.\n" );
    fflush( log );
  }
  fclose( log );
}

void ProfileManager::updateActiveProfileIndex()
{
  FILE *log = fopen( "/tmp/ucc-debug.log", "a" );
  fprintf( log, "updateActiveProfileIndex() called\n" );
  fprintf( log, "  m_activeProfile: %s\n", m_activeProfile.toStdString().c_str() );
  fprintf( log, "  m_activeProfileIndex (before): %d\n", m_activeProfileIndex );
  fflush( log );
  
  int newIndex = m_allProfiles.indexOf( m_activeProfile );
  fprintf( log, "  newIndex: %d\n", newIndex );
  fflush( log );

  if ( m_activeProfileIndex != newIndex )
  {
    fprintf( log, "  Active profile index changed! Emitting signal.\n" );
    fflush( log );
    m_activeProfileIndex = newIndex;
    emit activeProfileIndexChanged();
  }
  else
  {
    fprintf( log, "  Active profile index did NOT change.\n" );
    fflush( log );
  }
  fclose( log );
}

void ProfileManager::setActiveProfileByIndex( int index )
{
  if ( index >= 0 && index < m_allProfiles.size() )
  {
    setActiveProfile( m_allProfiles.at( index ) );
  }
}

std::vector< int > ProfileManager::getHardwarePowerLimits()
{
  // Return cached hardware limits
  qDebug() << "ProfileManager::getHardwarePowerLimits() returning:" << m_hardwarePowerLimits.size() << "values";
  for ( size_t i = 0; i < m_hardwarePowerLimits.size(); ++i )
  {
    qDebug() << "  Limit" << (int)i << "=" << m_hardwarePowerLimits[i];
  }
  return m_hardwarePowerLimits;
}

bool ProfileManager::isCustomProfile( const QString &profileName ) const
{
  // A profile is custom if it's in the custom profiles list
  return m_customProfiles.contains( profileName );
}

void ProfileManager::loadCustomProfilesFromSettings()
{
  m_customProfilesData = QJsonArray();
  m_customProfiles.clear();
  
  // Load custom profiles from QSettings
  QString profilesJson = m_settings->value("customProfiles", "{}").toString();
  qDebug() << "Loading custom profiles from settings, JSON:" << profilesJson;
  QJsonDocument doc = QJsonDocument::fromJson(profilesJson.toUtf8());
  
  if (doc.isArray()) {
    m_customProfilesData = doc.array();
    for (const QJsonValue &value : m_customProfilesData) {
      if (value.isObject()) {
        QJsonObject profileObj = value.toObject();
        QString name = profileObj.value("name").toString();
        if (!name.isEmpty()) {
          m_customProfiles.append(name);
          qDebug() << "Loaded custom profile:" << name;
        }
      }
    }
  }
  
  // Do not load or persist an 'activeProfile' in settings anymore.
  // Active profile will be determined from the stateMap + current power state.
  m_activeProfile = "";
  
  // Load stateMap from settings
  QString stateMapJson = m_settings->value("stateMap", "{}").toString();
  qDebug() << "Loading stateMap from settings, JSON:" << stateMapJson;
  QJsonDocument stateMapDoc = QJsonDocument::fromJson(stateMapJson.toUtf8());
  if (stateMapDoc.isObject()) {
    m_stateMap = stateMapDoc.object();
    qDebug() << "Loaded stateMap:" << m_stateMap;
  } else {
    // Default stateMap
    m_stateMap["power_ac"] = "__default_custom_profile__";
    m_stateMap["power_bat"] = "__default_custom_profile__";
  }
  
  qDebug() << "Loaded" << m_customProfiles.size() << "custom profiles from local storage";
}


void ProfileManager::loadCustomFanProfilesFromSettings()
{
  m_customFanProfilesData = QJsonArray();
  m_customFanProfiles.clear();

  QString fanJson = m_settings->value("customFanProfiles", "[]").toString();
  qDebug() << "Loading custom fan profiles from settings, JSON:" << fanJson;
  QJsonDocument doc = QJsonDocument::fromJson( fanJson.toUtf8() );

  if ( doc.isArray() )
  {
    m_customFanProfilesData = doc.array();
    for ( const auto &val : m_customFanProfilesData )
    {
      if ( val.isObject() )
      {
        QJsonObject o = val.toObject();
        QString name = o.value( "name" ).toString();
        if ( !name.isEmpty() )
        {
          m_customFanProfiles.append( name );
          qDebug() << "Loaded custom fan profile:" << name;
        }
      }
    }
  }
}

void ProfileManager::saveCustomFanProfilesToSettings()
{
  QJsonDocument doc( m_customFanProfilesData );
  QString jsonStr = doc.toJson( QJsonDocument::Compact );
  qDebug() << "Saving custom fan profiles to settings file:" << m_settings->fileName();
  qDebug() << "JSON:" << jsonStr;
  m_settings->setValue( "customFanProfiles", jsonStr );
  m_settings->sync();
}

void ProfileManager::saveCustomProfilesToSettings()
{
  QJsonDocument doc(m_customProfilesData);
  QString jsonStr = doc.toJson(QJsonDocument::Compact);
  qDebug() << "Saving custom profiles to settings file:" << m_settings->fileName();
  qDebug() << "JSON:" << jsonStr;
  m_settings->setValue("customProfiles", jsonStr);
  
  // Save stateMap
  QJsonDocument stateMapDoc(m_stateMap);
  QString stateMapJson = stateMapDoc.toJson(QJsonDocument::Compact);
  qDebug() << "Saving stateMap:" << stateMapJson;
  m_settings->setValue("stateMap", stateMapJson);
  
  m_settings->sync();
  qDebug() << "QSettings sync completed";
  
  qDebug() << "Saved" << m_customProfilesData.size() << "custom profiles to local storage";
}

// Helper: resolve a stateMap entry (which may be an id or name) to a profile name
QString ProfileManager::resolveStateMapToProfileName( const QString &state )
{
  if ( !m_stateMap.contains( state ) ) return QString();
  QString mapped = m_stateMap[state].toString();
  if ( mapped.isEmpty() ) return QString();

  // If mapped is already a profile name in our list, return it
  if ( m_allProfiles.contains( mapped ) || m_customProfiles.contains( mapped ) || m_defaultProfiles.contains( mapped ) )
    return mapped;

  // Otherwise, try to resolve as an ID
  for ( const auto &p : m_defaultProfilesData ) {
    if ( p.isObject() && p.toObject()["id"].toString() == mapped )
      return p.toObject()["name"].toString();
  }
  for ( const auto &p : m_customProfilesData ) {
    if ( p.isObject() && p.toObject()["id"].toString() == mapped )
      return p.toObject()["name"].toString();
  }

  return QString();
}

QString ProfileManager::getFanProfile( const QString &name )
{
  // If it's a custom fan profile stored locally, return it
  for ( const auto &v : m_customFanProfilesData )
  {
    if ( v.isObject() )
    {
      QJsonObject o = v.toObject();
      if ( o.value( "name" ).toString() == name )
      {
        QString jsonStr = o.value( "json" ).toString();

        // Diagnostic: inspect temperatures and spacing
        QJsonDocument doc = QJsonDocument::fromJson( jsonStr.toUtf8() );
        if ( doc.isObject() ) {
          QJsonObject obj = doc.object();
          if ( obj.contains("tableCPU") && obj["tableCPU"].isArray() ) {
            QJsonArray arr = obj["tableCPU"].toArray();
            QStringList temps;
            for ( int i = 0; i < arr.size() && i < 8; ++i ) {
              if ( arr[i].isObject() ) temps << QString::number( arr[i].toObject()["temp"].toInt() );
            }
            qDebug() << "[ProfileManager] Returning CUSTOM fan profile" << name << "CPU points:" << arr.size() << "sample temps:" << temps;

            // check spacing
            if ( arr.size() > 1 ) {
              int prev = arr[0].toObject()["temp"].toInt();
              for ( int i = 1; i < arr.size(); ++i ) {
                int t = arr[i].toObject()["temp"].toInt();
                int diff = t - prev;
                if ( diff % 5 != 0 || t < 20 || t > 100 ) {
                  qWarning() << "[ProfileManager] CUSTOM fan profile" << name << "has non-5°C spacing or out-of-range temp:" << t << "(diff" << diff << ")";
                  break;
                }
                prev = t;
              }
            }
          }
        }

        return jsonStr;
      }
    }
  }

  // Otherwise, fall back to daemon-provided built-in profiles via DBus
  if ( auto json = m_client->getFanProfile( name.toStdString() ) )
  {
    QString s = QString::fromStdString( *json );

    // Diagnostics: inspect built-in JSON we got from daemon
    QJsonDocument doc = QJsonDocument::fromJson( s.toUtf8() );
    if ( doc.isObject() ) {
      QJsonObject obj = doc.object();
      if ( obj.contains("tableCPU") && obj["tableCPU"].isArray() ) {
        QJsonArray arr = obj["tableCPU"].toArray();
        QStringList temps;
        for ( int i = 0; i < arr.size() && i < 8; ++i ) {
          if ( arr[i].isObject() ) temps << QString::number( arr[i].toObject()["temp"].toInt() );
        }
        qDebug() << "[ProfileManager] Returning BUILT-IN fan profile" << name << "CPU points:" << arr.size() << "sample temps:" << temps;

        // check spacing
        if ( arr.size() > 1 ) {
          int prev = arr[0].toObject()["temp"].toInt();
          for ( int i = 1; i < arr.size(); ++i ) {
            int t = arr[i].toObject()["temp"].toInt();
            int diff = t - prev;
            if ( diff % 5 != 0 || t < 20 || t > 100 ) {
              qWarning() << "[ProfileManager] BUILT-IN fan profile" << name << "has non-5°C spacing or out-of-range temp:" << t << "(diff" << diff << ")";
              break;
            }
            prev = t;
          }
        }
      }
    }

    return s;
  }
  return "{}";
}

bool ProfileManager::setFanProfile( const QString &name, const QString &json )
{
  // Do not allow overwriting built-in profiles
  if ( m_defaultProfiles.contains( name ) )
  {
    qWarning() << "Attempt to overwrite built-in fan profile:" << name;
    return false;
  }

  // Update existing entry or append new
  bool found = false;
  for ( int i = 0; i < m_customFanProfilesData.size(); ++i )
  {
    if ( m_customFanProfilesData[i].isObject() )
    {
      QJsonObject o = m_customFanProfilesData[i].toObject();
      if ( o.value( "name" ).toString() == name )
      {
        o[ "json" ] = json;
        m_customFanProfilesData[i] = o;
        found = true;
        break;
      }
    }
  }

  if ( !found )
  {
    QJsonObject o;
    o[ "name" ] = name;
    o[ "json" ] = json;
    m_customFanProfilesData.append( o );
    m_customFanProfiles.append( name );
  }

  saveCustomFanProfilesToSettings();
  return true;
}

bool ProfileManager::deleteFanProfile( const QString &name )
{
  bool removed = false;
  QJsonArray newArr;
  for ( const auto &v : m_customFanProfilesData )
  {
    if ( v.isObject() )
    {
      QJsonObject o = v.toObject();
      if ( o.value( "name" ).toString() == name )
      {
        removed = true;
        continue;
      }
    }
    newArr.append( v );
  }

  if ( removed )
  {
    m_customFanProfilesData = newArr;
    m_customFanProfiles.removeAll( name );
    saveCustomFanProfilesToSettings();
  }

  return removed;
}

QString ProfileManager::getSettingsJSON()
{
  try {
    if ( auto json = m_client->getSettingsJSON() )
    {
      return QString::fromStdString( *json );
    }
  } catch (const std::exception &e) {
    qWarning() << "Failed to get settings JSON:" << e.what();
  }
  return "{}";
}

bool ProfileManager::setStateMap( const QString &state, const QString &profileId )
{
  // Update local stateMap
  m_stateMap[state] = profileId;
  saveCustomProfilesToSettings();
  
  // Update tccd-ng
  return m_client->setStateMap( state.toStdString(), profileId.toStdString() );
}

} // namespace ucc
