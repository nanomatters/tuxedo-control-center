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

#include "SystemMonitor.hpp"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace ucc
{

SystemMonitor::SystemMonitor( QObject *parent )
  : QObject( parent )
  , m_client( std::make_unique< TccdClient >( this ) )
  , m_updateTimer( new QTimer( this ) )
{
  // Update metrics every 2 seconds
  connect( m_updateTimer, &QTimer::timeout, this, &SystemMonitor::updateMetrics );
  m_updateTimer->start( 2000 );

  // Initial update
  updateMetrics();
}

SystemMonitor::~SystemMonitor() = default;

void SystemMonitor::updateMetrics()
{
  // Get GPU info (includes temperature)
  if ( auto gpuInfo = m_client->getGpuInfo() )
  {
    QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( *gpuInfo ).toUtf8() );
    if ( doc.isObject() )
    {
      auto obj = doc.object();
      if ( obj.contains( "temperature" ) )
      {
        QString temp = QString::number( obj["temperature"].toInt() ) + "°C";
        if ( m_gpuTemp != temp )
        {
          m_gpuTemp = temp;
          emit gpuTempChanged();
        }
      }
    }
  }

  // Get display brightness
  if ( auto brightness = m_client->getDisplayBrightness() )
  {
    if ( m_displayBrightness != *brightness )
    {
      m_displayBrightness = *brightness;
      emit displayBrightnessChanged();
    }
  }

  // Get webcam status
  if ( auto enabled = m_client->getWebcamEnabled() )
  {
    if ( m_webcamEnabled != *enabled )
    {
      m_webcamEnabled = *enabled;
      emit webcamEnabledChanged();
    }
  }

  // Get Fn lock status
  if ( auto fnLock = m_client->getFnLock() )
  {
    if ( m_fnLock != *fnLock )
    {
      m_fnLock = *fnLock;
      emit fnLockChanged();
    }
  }
}

void SystemMonitor::setDisplayBrightness( int brightness )
{
  if ( m_client->setDisplayBrightness( brightness ) )
  {
    m_displayBrightness = brightness;
    emit displayBrightnessChanged();
  }
}

void SystemMonitor::setWebcamEnabled( bool enabled )
{
  if ( m_client->setWebcamEnabled( enabled ) )
  {
    m_webcamEnabled = enabled;
    emit webcamEnabledChanged();
  }
}

void SystemMonitor::setFnLock( bool enabled )
{
  if ( m_client->setFnLock( enabled ) )
  {
    m_fnLock = enabled;
    emit fnLockChanged();
  }
}

void SystemMonitor::refreshAll()
{
  updateMetrics();
}

} // namespace ucc
