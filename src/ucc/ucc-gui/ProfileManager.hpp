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

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <memory>
#include "TccdClient.hpp"

namespace ucc
{

/**
 * @brief Profile management for QML interface
 *
 * Provides profile operations with Qt signals/slots integration
 */
class ProfileManager : public QObject
{
  Q_OBJECT
  Q_PROPERTY( QStringList defaultProfiles READ defaultProfiles NOTIFY defaultProfilesChanged )
  Q_PROPERTY( QStringList customProfiles READ customProfiles NOTIFY customProfilesChanged )
  Q_PROPERTY( QStringList allProfiles READ allProfiles NOTIFY allProfilesChanged )
  Q_PROPERTY( QString activeProfile READ activeProfile NOTIFY activeProfileChanged )
  Q_PROPERTY( int activeProfileIndex READ activeProfileIndex NOTIFY activeProfileIndexChanged )
  Q_PROPERTY( bool connected READ isConnected NOTIFY connectedChanged )

public:
  explicit ProfileManager( QObject *parent = nullptr );
  ~ProfileManager() override = default;

  QStringList defaultProfiles() const { return m_defaultProfiles; }
  QStringList customProfiles() const { return m_customProfiles; }
  QStringList allProfiles() const { return m_allProfiles; }
  QString activeProfile() const { return m_activeProfile; }
  int activeProfileIndex() const { return m_activeProfileIndex; }
  bool isConnected() const { return m_connected; }

public slots:
  void refresh();
  void setActiveProfile( const QString &profileId );
  void setActiveProfileByIndex( int index );
  void saveProfile( const QString &profileJSON );
  void deleteProfile( const QString &profileId );
  QString getProfileDetails( const QString &profileId );
  QString getProfileIdByName( const QString &profileName );
  std::vector< int > getHardwarePowerLimits();
  bool isCustomProfile( const QString &profileName ) const;

signals:
  void defaultProfilesChanged();
  void customProfilesChanged();
  void allProfilesChanged();
  void activeProfileChanged();
  void activeProfileIndexChanged();
  void connectedChanged();
  void error( const QString &message );

private:
  void updateProfiles();
  void onProfileChanged( const std::string &profileId );
  void updateAllProfiles();
  void updateActiveProfileIndex();

  std::unique_ptr< TccdClient > m_client;
  QStringList m_defaultProfiles;
  QStringList m_customProfiles;
  QStringList m_allProfiles;
  QString m_activeProfile;
  int m_activeProfileIndex = -1;
  bool m_connected = false;
  std::vector< int > m_hardwarePowerLimits;

  QJsonArray m_defaultProfilesData;
  QJsonArray m_customProfilesData;
};

} // namespace ucc
