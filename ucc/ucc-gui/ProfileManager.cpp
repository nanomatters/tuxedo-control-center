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
  m_connected = m_client->isConnected();

  if ( m_connected )
  {
    // Connect to profile changed signal
    connect( m_client.get(), &TccdClient::profileChanged,
             this, [this]( const QString &profileId ) {
      onProfileChanged( profileId.toStdString() );
    } );

    // Load initial data
    updateProfiles();
  }

  emit connectedChanged();
}

void ProfileManager::refresh()
{
  updateProfiles();
}

void ProfileManager::updateProfiles()
{
  // Get default profiles
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
          m_defaultProfiles.append( name );
        }
      }
      emit defaultProfilesChanged();
    }
  }

  // Get custom profiles
  if ( auto json = m_client->getCustomProfilesJSON() )
  {
    QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( *json ).toUtf8() );
    if ( doc.isArray() )
    {
      m_customProfilesData = doc.array();
      m_customProfiles.clear();
      for ( const auto &profile : m_customProfilesData )
      {
        if ( profile.isObject() )
        {
          QString name = profile.toObject()["name"].toString();
          m_customProfiles.append( name );
        }
      }
      emit customProfilesChanged();
    }
  }

  // Get active profile
  if ( auto json = m_client->getActiveProfileJSON() )
  {
    QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( *json ).toUtf8() );
    if ( doc.isObject() )
    {
      QString name = doc.object()["name"].toString();
      if ( m_activeProfile != name )
      {
        m_activeProfile = name;
        emit activeProfileChanged();
      }
    }
  }
}

void ProfileManager::setActiveProfile( const QString &profileId )
{
  if ( m_client->setActiveProfile( profileId.toStdString() ) )
  {
    qDebug() << "Profile activated:" << profileId;
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

void ProfileManager::onProfileChanged( const std::string &profileId )
{
  qDebug() << "Profile changed signal:" << QString::fromStdString( profileId );
  updateProfiles();
}

} // namespace ucc
