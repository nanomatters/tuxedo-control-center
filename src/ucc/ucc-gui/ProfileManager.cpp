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

namespace ucc
{

ProfileManager::ProfileManager( QObject *parent )
  : QObject( parent )
  , m_client( std::make_unique< TccdClient >( this ) )
{
  FILE *log = fopen( "/tmp/ucc-debug.log", "a" );
  fprintf( log, "ProfileManager constructor called\n" );
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
  
  // Get default profiles
  if ( auto json = m_client->getDefaultProfilesJSON() )
  {
    fprintf( log, "Got default profiles JSON\n" );
    fflush( log );
    
    QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( *json ).toUtf8() );
    if ( doc.isArray() )
    {
      m_defaultProfilesData = doc.array();
      m_defaultProfiles.clear();
      fprintf( log, "Default profiles count: %ld\n", (long)m_defaultProfilesData.size() );
      fflush( log );
      
      for ( const auto &profile : m_defaultProfilesData )
      {
        if ( profile.isObject() )
        {
          QString name = profile.toObject()["name"].toString();
          m_defaultProfiles.append( name );
          fprintf( log, "  Added default profile: %s\n", name.toStdString().c_str() );
          fflush( log );
        }
      }
      emit defaultProfilesChanged();
    }
  }
  else
  {
    fprintf( log, "ERROR: Failed to get default profiles JSON\n" );
    fflush( log );
  }

  // Get custom profiles
  if ( auto json = m_client->getCustomProfilesJSON() )
  {
    fprintf( log, "Got custom profiles JSON\n" );
    fflush( log );
    
    QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( *json ).toUtf8() );
    if ( doc.isArray() )
    {
      m_customProfilesData = doc.array();
      m_customProfiles.clear();
      fprintf( log, "Custom profiles count: %ld\n", (long)m_customProfilesData.size() );
      fflush( log );
      
      for ( const auto &profile : m_customProfilesData )
      {
        if ( profile.isObject() )
        {
          QString name = profile.toObject()["name"].toString();
          m_customProfiles.append( name );
          fprintf( log, "  Added custom profile: %s\n", name.toStdString().c_str() );
          fflush( log );
        }
      }
      emit customProfilesChanged();
    }
  }
  else
  {
    fprintf( log, "ERROR: Failed to get custom profiles JSON\n" );
    fflush( log );
  }

  // Get active profile
  if ( auto json = m_client->getActiveProfileJSON() )
  {
    fprintf( log, "Got active profile JSON\n" );
    fflush( log );
    
    QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( *json ).toUtf8() );
    if ( doc.isObject() )
    {
      QString name = doc.object()["name"].toString();
      fprintf( log, "Active profile name: %s\n", name.toStdString().c_str() );
      fflush( log );
      
      if ( m_activeProfile != name )
      {
        m_activeProfile = name;
        emit activeProfileChanged();
      }
    }
  }
  else
  {
    fprintf( log, "ERROR: Failed to get active profile JSON\n" );
    fflush( log );
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
  
  if ( m_client->setActiveProfile( actualId.toStdString() ) )
  {
    qDebug() << "Profile activated:" << profileId << "(ID:" << actualId << ")";
    updateProfiles();
  }
  else
  {
    emit error( "Failed to activate profile: " + profileId );
  }
}

void ProfileManager::saveProfile( const QString &profileJSON )
{
  if ( m_client->saveCustomProfile( profileJSON.toStdString() ) )
  {
    qDebug() << "Profile saved successfully";
    updateProfiles();
  }
  else
  {
    emit error( "Failed to save profile" );
  }
}

void ProfileManager::deleteProfile( const QString &profileId )
{
  if ( m_client->deleteCustomProfile( profileId.toStdString() ) )
  {
    qDebug() << "Profile deleted:" << profileId;
    updateProfiles();
  }
  else
  {
    emit error( "Failed to delete profile: " + profileId );
  }
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

} // namespace ucc
