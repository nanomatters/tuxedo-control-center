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
}

void ProfileManager::refresh()
{
  updateProfiles();
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

  // Custom profiles are loaded from local storage, no need to fetch from server
  emit customProfilesChanged();

  // Get active profile - but prefer locally stored active profile
  if (m_activeProfile.isEmpty() || !m_allProfiles.contains(m_activeProfile)) {
    if ( auto json = m_client->getActiveProfileJSON() )
    {
      fprintf( log, "Got active profile JSON from server\n" );
      fflush( log );
      
      QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( *json ).toUtf8() );

      if ( doc.isObject() )
      {
        QString name = doc.object()["name"].toString();
        fprintf( log, "Active profile name from server: %s\n", name.toStdString().c_str() );
        fflush( log );
        

        if ( m_activeProfile != name )
        {
          m_activeProfile = name;
          m_settings->setValue("activeProfile", name);
          emit activeProfileChanged();
        }
      }
    }
    else
    {
      fprintf( log, "ERROR: Failed to get active profile JSON from server\n" );
      fflush( log );
      // If we can't get from server and don't have a valid local one, default to "Default"
      if (m_activeProfile.isEmpty() || !m_allProfiles.contains(m_activeProfile)) {
        m_activeProfile = "Default";
        m_settings->setValue("activeProfile", "Default");
        fprintf( log, "Setting default active profile: Default\n" );
        fflush( log );
      }
    }
  } else {
    fprintf( log, "Using locally stored active profile: %s\n", m_activeProfile.toStdString().c_str() );
    fflush( log );
  }

  // Apply mains profile since UCC starts in mains mode
  QString mainsProfile = m_stateMap["power_ac"].toString();
  if ( !mainsProfile.isEmpty() && m_allProfiles.contains( mainsProfile ) )
  {
    fprintf( log, "Applying mains profile: %s\n", mainsProfile.toStdString().c_str() );
    fflush( log );
    setActiveProfile( mainsProfile );
  }

  // Update combined list and index
  fprintf( log, "Calling updateAllProfiles()\n" );
  fflush( log );
  updateAllProfiles();
  updateActiveProfileIndex();
  fprintf( log, "updateProfiles() done\n" );
  fflush( log );
  fclose( log );
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
    m_settings->setValue("activeProfile", profileId);
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
  
  // Check if profile already exists
  bool exists = false;
  for (int i = 0; i < m_customProfilesData.size(); ++i) {
    QJsonObject existingProfile = m_customProfilesData[i].toObject();
    if (existingProfile.value("id").toString() == profileId) {
      // Update existing profile
      m_customProfilesData[i] = profileObj;
      exists = true;
      break;
    }
  }
  
  if (!exists) {
    // Add new profile
    m_customProfilesData.append(profileObj);
    m_customProfiles.append(profileName);
  } else {
    // Update name in the list if it changed
    int index = m_customProfiles.indexOf(profileName);
    if (index == -1) {
      // Name changed, update the list
      for (int i = 0; i < m_customProfilesData.size(); ++i) {
        QJsonObject existingProfile = m_customProfilesData[i].toObject();
        if (existingProfile.value("id").toString() == profileId) {
          QString oldName = existingProfile.value("name").toString();
          m_customProfiles.replace(m_customProfiles.indexOf(oldName), profileName);
          break;
        }
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
  
  QString profileId = m_stateMap[state].toString();
  
  if ( profileId.isEmpty() )
  {
    qWarning() << "No profile mapped for state:" << state;
    return;
  }
  
  qDebug() << "Applying profile" << profileId << "for power state" << state;
  
  QString profileJson = getProfileDetails( profileId );
  if ( !profileJson.isEmpty() )
  {
    try {
      m_client->applyProfile( profileJson.toStdString() );
    } catch (const std::exception &e) {
      qWarning() << "Failed to apply profile:" << e.what();
      return;
    }
    // Update active profile
    for ( const auto &profile : m_defaultProfilesData )
    {
      if ( profile.toObject()["id"].toString() == profileId )
      {
        m_activeProfile = profile.toObject()["name"].toString();
        emit activeProfileChanged();
        return;
      }
    }
    for ( const auto &profile : m_customProfilesData )
    {
      if ( profile.toObject()["id"].toString() == profileId )
      {
        m_activeProfile = profile.toObject()["name"].toString();
        emit activeProfileChanged();
        return;
      }
    }
  }
  else
  {
    qWarning() << "Profile not found:" << profileId;
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
  
  // Load active profile from settings
  QString activeProfileName = m_settings->value("activeProfile", "Default").toString();
  if (m_allProfiles.contains(activeProfileName) || m_customProfiles.contains(activeProfileName)) {
    m_activeProfile = activeProfileName;
    qDebug() << "Loaded active profile from settings:" << m_activeProfile;
  } else {
    m_activeProfile = "Default"; // fallback
    qDebug() << "Active profile from settings not found, using Default";
  }
  
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

QString ProfileManager::getFanProfile( const QString &name )
{
  if ( auto json = m_client->getFanProfile( name.toStdString() ) )
  {
    return QString::fromStdString( *json );
  }
  return "{}";
}

bool ProfileManager::setFanProfile( const QString &name, const QString &json )
{
  if ( auto result = m_client->setFanProfile( name.toStdString(), json.toStdString() ) )
  {
    return result.value();
  }
  return false;
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
