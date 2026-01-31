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
#include "UccdClient.hpp"

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
  Q_PROPERTY( QString cpuTemp READ cpuTemp NOTIFY cpuTempChanged )
  Q_PROPERTY( QString cpuFrequency READ cpuFrequency NOTIFY cpuFrequencyChanged )
  Q_PROPERTY( QString cpuPower READ cpuPower NOTIFY cpuPowerChanged )
  Q_PROPERTY( QString gpuTemp READ gpuTemp NOTIFY gpuTempChanged )
  Q_PROPERTY( QString gpuFrequency READ gpuFrequency NOTIFY gpuFrequencyChanged )
  Q_PROPERTY( QString gpuPower READ gpuPower NOTIFY gpuPowerChanged )
  Q_PROPERTY( QString cpuFanSpeed READ cpuFanSpeed NOTIFY fanSpeedChanged )
  Q_PROPERTY( QString gpuFanSpeed READ gpuFanSpeed NOTIFY gpuFanSpeedChanged )
  Q_PROPERTY( int displayBrightness READ displayBrightness WRITE setDisplayBrightness NOTIFY displayBrightnessChanged )
  Q_PROPERTY( bool webcamEnabled READ webcamEnabled WRITE setWebcamEnabled NOTIFY webcamEnabledChanged )
  Q_PROPERTY( bool fnLock READ fnLock WRITE setFnLock NOTIFY fnLockChanged )
  Q_PROPERTY( bool monitoringActive READ monitoringActive WRITE setMonitoringActive NOTIFY monitoringActiveChanged )

public:
  explicit SystemMonitor( QObject *parent = nullptr );
  ~SystemMonitor() override;

  QString cpuUsage() const { return m_cpuUsage; }
  QString cpuTemp() const { return m_cpuTemp; }
  QString cpuFrequency() const { return m_cpuFrequency; }
  QString cpuPower() const { return m_cpuPower; }
  QString gpuTemp() const { return m_gpuTemp; }
  QString gpuFrequency() const { return m_gpuFrequency; }
  QString gpuPower() const { return m_gpuPower; }
  QString cpuFanSpeed() const { return m_fanSpeed; }
  QString gpuFanSpeed() const { return m_gpuFanSpeed; }
  int displayBrightness() const { return m_displayBrightness; }
  bool webcamEnabled() const { return m_webcamEnabled; }
  bool fnLock() const { return m_fnLock; }
  bool monitoringActive() const { return m_monitoringActive; }

public slots:
  void setDisplayBrightness( int brightness );
  void setWebcamEnabled( bool enabled );
  void setFnLock( bool enabled );
  void setMonitoringActive( bool active );
  void refreshAll();

signals:
  void cpuUsageChanged();
  void cpuTempChanged();
  void cpuFrequencyChanged();
  void cpuPowerChanged();
  void gpuTempChanged();
  void gpuFrequencyChanged();
  void gpuPowerChanged();
  void fanSpeedChanged();
  void gpuFanSpeedChanged();
  void displayBrightnessChanged();
  void webcamEnabledChanged();
  void fnLockChanged();
  void monitoringActiveChanged();

private slots:
  void updateMetrics();

private:
  std::unique_ptr< UccdClient > m_client;
  QTimer *m_updateTimer;

  QString m_cpuUsage = "0%";
  QString m_cpuTemp = "0°C";
  QString m_cpuFrequency = "0 MHz";
  QString m_cpuPower = "0 W";
  QString m_gpuTemp = "0°C";
  QString m_gpuFrequency = "0 MHz";
  QString m_gpuPower = "0 W";
  QString m_fanSpeed = "0 RPM";
  QString m_gpuFanSpeed = "0 RPM";
  int m_displayBrightness = 50;
  bool m_webcamEnabled = true;
  bool m_fnLock = false;
  bool m_monitoringActive = false;
};

} // namespace ucc
