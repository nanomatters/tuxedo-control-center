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

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

namespace ucc
{
  class SystemMonitor;
  class ProfileManager;

  /**
   * @brief Dashboard tab widget for system monitoring
   */
  class DashboardTab : public QWidget
  {
    Q_OBJECT

  public:
    explicit DashboardTab( SystemMonitor *systemMonitor, ProfileManager *profileManager, QWidget *parent = nullptr );
    ~DashboardTab() override = default;

  private slots:
    void onCpuTempChanged();
    void onCpuFrequencyChanged();
    void onCpuPowerChanged();
    void onGpuTempChanged();
    void onGpuFrequencyChanged();
    void onGpuPowerChanged();
    void onFanSpeedChanged();
    void onGpuFanSpeedChanged();

  private:
    void setupUI();
    void connectSignals();

    SystemMonitor *m_systemMonitor;
    ProfileManager *m_profileManager;

    // Dashboard widgets
    QLabel *m_activeProfileLabel = nullptr;
    QLabel *m_cpuTempLabel = nullptr;
    QLabel *m_cpuFrequencyLabel = nullptr;
    QLabel *m_gpuTempLabel = nullptr;
    QLabel *m_gpuFrequencyLabel = nullptr;
    QLabel *m_fanSpeedLabel = nullptr;
    QLabel *m_gpuFanSpeedLabel = nullptr;
    QLabel *m_cpuPowerLabel = nullptr;
    QLabel *m_gpuPowerLabel = nullptr;
  };
}