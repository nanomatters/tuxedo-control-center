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
#include <QTimer>
#include <memory>
#include "TccdClient.hpp"

namespace ucc
{

/**
 * @brief System monitoring for QML interface
 *
 * Provides real-time system metrics
 */
class SystemMonitor : public QObject
{
  Q_OBJECT
  Q_PROPERTY( QString cpuUsage READ cpuUsage NOTIFY cpuUsageChanged )
  Q_PROPERTY( QString gpuTemp READ gpuTemp NOTIFY gpuTempChanged )
  Q_PROPERTY( QString fanSpeed READ fanSpeed NOTIFY fanSpeedChanged )
  Q_PROPERTY( int displayBrightness READ displayBrightness WRITE setDisplayBrightness NOTIFY displayBrightnessChanged )
  Q_PROPERTY( bool webcamEnabled READ webcamEnabled WRITE setWebcamEnabled NOTIFY webcamEnabledChanged )
  Q_PROPERTY( bool fnLock READ fnLock WRITE setFnLock NOTIFY fnLockChanged )

public:
  explicit SystemMonitor( QObject *parent = nullptr );
  ~SystemMonitor() override;

  QString cpuUsage() const { return m_cpuUsage; }
  QString gpuTemp() const { return m_gpuTemp; }
  QString fanSpeed() const { return m_fanSpeed; }
  int displayBrightness() const { return m_displayBrightness; }
  bool webcamEnabled() const { return m_webcamEnabled; }
  bool fnLock() const { return m_fnLock; }

public slots:
  void setDisplayBrightness( int brightness );
  void setWebcamEnabled( bool enabled );
  void setFnLock( bool enabled );
  void refreshAll();

signals:
  void cpuUsageChanged();
  void gpuTempChanged();
  void fanSpeedChanged();
  void displayBrightnessChanged();
  void webcamEnabledChanged();
  void fnLockChanged();

private slots:
  void updateMetrics();

private:
  std::unique_ptr< TccdClient > m_client;
  QTimer *m_updateTimer;

  QString m_cpuUsage = "0%";
  QString m_gpuTemp = "0°C";
  QString m_fanSpeed = "0 RPM";
  int m_displayBrightness = 50;
  bool m_webcamEnabled = true;
  bool m_fnLock = false;
};

} // namespace ucc
