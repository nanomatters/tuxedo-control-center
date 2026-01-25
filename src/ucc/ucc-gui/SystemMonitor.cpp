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
  // Update metrics every 500ms when active
  connect( m_updateTimer, &QTimer::timeout, this, &SystemMonitor::updateMetrics );
  m_updateTimer->setInterval( 500 );

  // Timer will be started when monitoring becomes active
}

SystemMonitor::~SystemMonitor() = default;

void SystemMonitor::updateMetrics()
{
  qDebug() << "[SystemMonitor] updateMetrics() called";
  
  // Get CPU Temperature
  {
    QString cpuTemp = "--";

    if ( auto temp = m_client->getCpuTemperature() )
    {
      qDebug() << "[SystemMonitor] CPU temp:" << *temp;
      cpuTemp = QString::number( *temp ) + "°C";
    }
    else
    {
      qDebug() << "[SystemMonitor] Failed to get CPU temperature";
    }

    if ( m_cpuTemp != cpuTemp )
    {
      m_cpuTemp = cpuTemp;
      emit cpuTempChanged();
    }
  }

  // Get CPU Frequency
  {
    QString cpuFreq = "--";

    if ( auto freq = m_client->getCpuFrequency() )
    {
      cpuFreq = QString::number( *freq ) + " MHz";
    }

    if ( m_cpuFrequency != cpuFreq )
    {
      m_cpuFrequency = cpuFreq;
      emit cpuFrequencyChanged();
    }
  }

  // Get CPU Power
  {
    QString cpuPow = "--";

    if ( auto power = m_client->getCpuPower() )
    {
      cpuPow = QString::number( *power, 'f', 1 ) + " W";
    }

    if ( m_cpuPower != cpuPow )
    {
      m_cpuPower = cpuPow;
      emit cpuPowerChanged();
    }
  }

  // Get GPU Temperature
  {
    QString gpuTemp = "--";

    if ( auto temp = m_client->getGpuTemperature() )
    {
      qDebug() << "[SystemMonitor] GPU temp:" << *temp;
      gpuTemp = QString::number( *temp ) + "°C";
    }
    else
    {
      qDebug() << "[SystemMonitor] Failed to get GPU temperature";
    }
    if ( m_gpuTemp != gpuTemp )
    {
      m_gpuTemp = gpuTemp;
      emit gpuTempChanged();
    }
  }

  // Get GPU Frequency
  {
    QString gpuFreq = "--";

    if ( auto freq = m_client->getGpuFrequency() )
    {
      qDebug() << "[SystemMonitor] GPU freq:" << *freq;
      gpuFreq = QString::number( *freq ) + " MHz";
    }
    else
    {
      qDebug() << "[SystemMonitor] Failed to get GPU frequency";
    }

    if ( m_gpuFrequency != gpuFreq )
    {
      m_gpuFrequency = gpuFreq;
      emit gpuFrequencyChanged();
    }
  }

  // Get GPU Power
  {
    QString gpuPow = "--";

    if ( auto power = m_client->getGpuPower() )
    {
      gpuPow = QString::number( *power, 'f', 1 ) + " W";
    }
    if ( m_gpuPower != gpuPow )
    {
      m_gpuPower = gpuPow;
      emit gpuPowerChanged();
    }
  }

  // Get Fan Speed
  {
    QString fanSpd = "--";

    if ( auto rpm = m_client->getFanSpeedRPM() )
    {
      qDebug() << "[SystemMonitor] Fan speed:" << *rpm << "RPM";
      fanSpd = QString::number( *rpm ) + " RPM";
    }
    else
    {
      qDebug() << "[SystemMonitor] Failed to get fan speed";
    }

    if ( m_fanSpeed != fanSpd )
    {
      m_fanSpeed = fanSpd;
      emit fanSpeedChanged();
    }
  }

  // Get GPU Fan Speed
  {
    QString fanSpd = "--";

    if ( auto rpm = m_client->getGpuFanSpeedRPM() )
    {
      qDebug() << "[SystemMonitor] GPU fan speed:" << *rpm << "RPM";
      fanSpd = QString::number( *rpm ) + " RPM";
    }
    else
    {
      qDebug() << "[SystemMonitor] Failed to get GPU fan speed";
    }
    if ( m_gpuFanSpeed != fanSpd )
    {
      m_gpuFanSpeed = fanSpd;
      emit gpuFanSpeedChanged();
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

void SystemMonitor::setMonitoringActive( bool active )
{
  if ( m_monitoringActive != active )
  {
    m_monitoringActive = active;
    emit monitoringActiveChanged();

    if ( active )
    {
      // Start monitoring - do immediate update and start timer
      qDebug() << "[SystemMonitor] Starting monitoring with 500ms interval";
      
      // Force refresh all values by clearing them first
      m_cpuTemp = "";
      m_cpuFrequency = "";
      m_cpuPower = "";
      m_gpuTemp = "";
      m_gpuFrequency = "";
      m_gpuPower = "";
      m_fanSpeed = "";
      m_gpuFanSpeed = "";
      
      // Start timer first - this gives tccd-ng time to collect initial sensor data
      // The first update will happen after 500ms
      m_updateTimer->start();
      qDebug() << "[SystemMonitor] Timer started, first update in 500ms. Timer active:" << m_updateTimer->isActive();
    }
    else
    {
      // Stop monitoring
      qDebug() << "[SystemMonitor] Stopping monitoring";
      m_updateTimer->stop();
    }
  }
}

} // namespace ucc
