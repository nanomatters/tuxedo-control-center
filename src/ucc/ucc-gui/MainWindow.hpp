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

#include <QMainWindow>
#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QTabWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QSpinBox>
#include <memory>
#include "ProfileManager.hpp"
#include "SystemMonitor.hpp"
#include "FanCurveEditorWidget.hpp"
#include "../libucc-dbus/TccdClient.hpp"

namespace ucc
{
  /**
   * @brief Main application window with C++ Qt widgets
   */
  class MainWindow : public QMainWindow
  {
    Q_OBJECT

  public:
    explicit MainWindow( QWidget *parent = nullptr );
    ~MainWindow() override;

  private slots:
    // Profile page slots
    void onProfileIndexChanged( int index );
    void onAllProfilesChanged();
    void onActiveProfileIndexChanged();
    void onBrightnessSliderChanged( int value );
    void onMinFanSpeedChanged( int value );
    void onMaxFanSpeedChanged( int value );
    void onOffsetFanSpeedChanged( int value );
    void onCpuCoresChanged( int value );
    void onMaxFrequencyChanged( int value );
    void onODMPowerLimit1Changed( int value );
    void onODMPowerLimit2Changed( int value );
    void onODMPowerLimit3Changed( int value );
    void onGpuPowerChanged( int value );
    void onApplyClicked();
    void onSaveClicked();

    // Dashboard page slots
    void onCpuTempChanged();
    void onCpuFrequencyChanged();
    void onCpuPowerChanged();
    void onGpuTempChanged();
    void onGpuFrequencyChanged();
    void onGpuPowerDashboardChanged();
    void onFanSpeedChanged();
    void onGpuFanSpeedChanged();
    void onDisplayBrightnessSliderChanged( int value );
    void onWebcamToggled( bool checked );
    void onFnLockToggled( bool checked );
    void onTabChanged( int index );

  private:
    void setupUI();
    void setupDashboardPage();
    void setupProfilesPage();
    void setupPerformancePage();
    void setupHardwarePage();
    void connectSignals();
    void loadProfileDetails( const QString &profileName );
    void markChanged();
    void updateButtonStates();
    void setupFanControlTab();

    std::unique_ptr< ProfileManager > m_profileManager;
    std::unique_ptr< SystemMonitor > m_systemMonitor;
    std::unique_ptr< TccdClient > m_tccdClient;

    // Tab widget
    QTabWidget *m_tabs = nullptr;

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
    QSlider *m_displayBrightnessSlider = nullptr;
    QLabel *m_displayBrightnessValueLabel = nullptr;
    QCheckBox *m_webcamCheckBox = nullptr;
    QCheckBox *m_fnLockCheckBox = nullptr;
    QLabel *m_statusLabel = nullptr;

    // Profiles widgets
    QComboBox *m_profileCombo = nullptr;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QTextEdit *m_descriptionEdit = nullptr;
    QPushButton *m_mainsButton = nullptr;
    QPushButton *m_batteryButton = nullptr;
    
    // Display controls
    QCheckBox *m_setBrightnessCheckBox = nullptr;
    QSlider *m_brightnessSlider = nullptr;
    QLabel *m_brightnessValueLabel = nullptr;
    
    // Fan control widgets
    QComboBox *m_fanProfileCombo = nullptr;
    QSlider *m_minFanSpeedSlider = nullptr;
    QLabel *m_minFanSpeedValue = nullptr;
    QSlider *m_maxFanSpeedSlider = nullptr;
    QLabel *m_maxFanSpeedValue = nullptr;
    QSlider *m_offsetFanSpeedSlider = nullptr;
    QLabel *m_offsetFanSpeedValue = nullptr;
    // Fan curve editor widgets
    FanCurveEditorWidget *m_cpuFanCurveEditor = nullptr;
    FanCurveEditorWidget *m_gpuFanCurveEditor = nullptr;
    
    // CPU frequency control widgets
    QSlider *m_cpuCoresSlider = nullptr;
    QLabel *m_cpuCoresValue = nullptr;
    QCheckBox *m_maxPerformanceCheckBox = nullptr;
    QSlider *m_minFrequencySlider = nullptr;
    QLabel *m_minFrequencyValue = nullptr;
    QSlider *m_maxFrequencySlider = nullptr;
    QLabel *m_maxFrequencyValue = nullptr;
    
    // ODM Power Limit (TDP) widgets
    QSlider *m_odmPowerLimit1Slider = nullptr;
    QLabel *m_odmPowerLimit1Value = nullptr;
    QSlider *m_odmPowerLimit2Slider = nullptr;
    QLabel *m_odmPowerLimit2Value = nullptr;
    QSlider *m_odmPowerLimit3Slider = nullptr;
    QLabel *m_odmPowerLimit3Value = nullptr;
    
    // GPU power control
    QSlider *m_gpuPowerSlider = nullptr;
    QLabel *m_gpuPowerValue = nullptr;
    
    // Change tracking
    bool m_profileChanged = false;
    QString m_currentLoadedProfile;
  };
}
