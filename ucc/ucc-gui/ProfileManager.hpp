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
  Q_PROPERTY( QString activeProfile READ activeProfile NOTIFY activeProfileChanged )
  Q_PROPERTY( bool connected READ isConnected NOTIFY connectedChanged )

public:
  explicit ProfileManager( QObject *parent = nullptr );
  ~ProfileManager() override = default;

  QStringList defaultProfiles() const { return m_defaultProfiles; }
  QStringList customProfiles() const { return m_customProfiles; }
  QString activeProfile() const { return m_activeProfile; }
  bool isConnected() const { return m_connected; }

public slots:
  void refresh();
  void setActiveProfile( const QString &profileId );
  void saveProfile( const QString &profileJSON );
  void deleteProfile( const QString &profileId );
  QString getProfileDetails( const QString &profileId );

signals:
  void defaultProfilesChanged();
  void customProfilesChanged();
  void activeProfileChanged();
  void connectedChanged();
  void error( const QString &message );

private:
  void updateProfiles();
  void onProfileChanged( const std::string &profileId );

  std::unique_ptr< TccdClient > m_client;
  QStringList m_defaultProfiles;
  QStringList m_customProfiles;
  QString m_activeProfile;
  bool m_connected = false;

  QJsonArray m_defaultProfilesData;
  QJsonArray m_customProfilesData;
};

} // namespace ucc
