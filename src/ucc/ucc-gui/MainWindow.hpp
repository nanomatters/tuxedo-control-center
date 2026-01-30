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
#include <QStatusBar>
#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTabWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QLineEdit>
#include <QInputDialog>
#include <QColorDialog>
#include <QStackedWidget>
#include <QtWidgets/QTableWidget>
#include <memory>
#include "ProfileManager.hpp"
#include "SystemMonitor.hpp"
#include "../libucc-dbus/UccdClient.hpp"
#include "FanCurveEditorWidget.hpp"
#include "KeyboardVisualizerWidget.hpp"

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
    void onCustomKeyboardProfilesChanged();
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
    void onApplyFanProfilesClicked();
    void onSaveFanProfilesClicked();
    void onRevertFanProfilesClicked();
    void onAddProfileClicked();
    void onCopyProfileClicked();
    void onRemoveProfileClicked();
    void onAddFanProfileClicked();
    void onRemoveFanProfileClicked();
    void onCpuFanPointsChanged(const QVector<FanCurveEditorWidget::Point>& points);
    void onGpuFanPointsChanged(const QVector<FanCurveEditorWidget::Point>& points);
    void onFanProfileChanged(const QString& profileName);
    void onCopyFanProfileClicked();

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
    void onKeyboardBrightnessChanged( int value );
    void onKeyboardColorClicked();
    void onKeyboardVisualizerColorsChanged();
    void onKeyboardColorModeChanged( int id );
    void onKeyboardProfileChanged(const QString& profileName);
    void onAddKeyboardProfileClicked();
    void onCopyKeyboardProfileClicked();
    void onSaveKeyboardProfileClicked();
    void onRemoveKeyboardProfileClicked();
    void reloadKeyboardProfiles();
    void updateKeyboardProfileButtonStates();

  private:
    struct FanPoint {
        int temp;
        int speed;
    };
    
    void setupUI();
    void setupDashboardPage();
    void setupProfilesPage();
    void setupHardwarePage();
    void setupKeyboardBacklightPage();
    void connectKeyboardBacklightPageWidgets();
    void loadFanPoints();
    void saveFanPoints();
    void connectSignals();

    // Update fan profile combo from daemon and custom store
    void reloadFanProfiles();

    // Slot: called when DBus connection status changes
    void onUccdConnectionChanged( bool connected );
    void loadProfileDetails( const QString &profileName );
    void markChanged();
    void updateButtonStates();
    void setupFanControlTab();
    void updateFanTabVisibility();
    void updateProfileEditingWidgets( bool isCustom );

    std::unique_ptr< ProfileManager > m_profileManager;
    std::unique_ptr< SystemMonitor > m_systemMonitor;
    std::unique_ptr< UccdClient > m_UccdClient;

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

    // Profiles widgets
    QComboBox *m_profileCombo = nullptr;
    int m_selectedProfileIndex = -1;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_copyProfileButton = nullptr;
    QPushButton *m_removeProfileButton = nullptr;
    QTextEdit *m_descriptionEdit = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QPushButton *m_mainsButton = nullptr;
    QPushButton *m_batteryButton = nullptr;
    
    constexpr bool profileTopWidgetsAvailable() const
    { return m_applyButton && m_saveButton && m_copyProfileButton && m_removeProfileButton && m_profileCombo; }

    // Display controls
    QCheckBox *m_setBrightnessCheckBox = nullptr;
    QSlider *m_brightnessSlider = nullptr;
    QLabel *m_brightnessValueLabel = nullptr;
    QComboBox *m_profileKeyboardProfileCombo = nullptr;
    
    // Fan control widgets
    QSlider *m_minFanSpeedSlider = nullptr;
    QLabel *m_minFanSpeedValue = nullptr;
    QSlider *m_maxFanSpeedSlider = nullptr;
    QLabel *m_maxFanSpeedValue = nullptr;
    QSlider *m_offsetFanSpeedSlider = nullptr;
    QLabel *m_offsetFanSpeedValue = nullptr;
    QCheckBox *m_sameFanSpeedCheckBox = nullptr;
    // Fan curve editor widgets
    QComboBox *m_fanProfileCombo = nullptr;
    QComboBox *m_profileFanProfileCombo = nullptr;
    FanCurveEditorWidget *m_cpuFanCurveEditor = nullptr;
    FanCurveEditorWidget *m_gpuFanCurveEditor = nullptr;
    QPushButton *m_applyFanProfilesButton = nullptr;
    QPushButton *m_saveFanProfilesButton = nullptr;
    QPushButton *m_revertFanProfilesButton = nullptr;
    QPushButton *m_addFanProfileButton = nullptr;
    QPushButton *m_copyFanProfileButton = nullptr;
    // List of built-in fan profile names provided by uccd
    QStringList m_builtinFanProfiles; 
    QPushButton *m_removeFanProfileButton = nullptr;
    QVector<FanPoint> m_cpuFanPoints;
    QVector<FanPoint> m_gpuFanPoints;
    
    constexpr bool fanProfileTopWidgetsAvailable() const
    {
      return m_applyFanProfilesButton && m_saveFanProfilesButton && m_copyFanProfileButton &&
             m_removeFanProfileButton && m_fanProfileCombo && m_revertFanProfilesButton;
    }

    // Fan tab
    QWidget *m_fanTab = nullptr;
    
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
    
    // Keyboard backlight widgets
    QSlider *m_keyboardBrightnessSlider = nullptr;
    QLabel *m_keyboardBrightnessValueLabel = nullptr;
    QPushButton *m_keyboardColorButton = nullptr;
    KeyboardVisualizerWidget *m_keyboardVisualizer = nullptr;
    
    // Keyboard profile widgets
    QComboBox *m_keyboardProfileCombo = nullptr;
    QPushButton *m_addKeyboardProfileButton = nullptr;
    QPushButton *m_copyKeyboardProfileButton = nullptr;
    QPushButton *m_saveKeyboardProfileButton = nullptr;
    QPushButton *m_removeKeyboardProfileButton = nullptr;
    
    // Keyboard color mode widgets
    QButtonGroup *m_keyboardColorModeGroup = nullptr;
    QRadioButton *m_keyboardGlobalColorRadio = nullptr;
    QRadioButton *m_keyboardPerKeyColorRadio = nullptr;
    QLabel *m_keyboardColorLabel = nullptr;
    QLabel *m_keyboardVisualizerLabel = nullptr;
    
    // Change tracking
    bool m_profileChanged = false;
    QString m_currentLoadedProfile;
    QString m_currentFanProfile;
    bool m_loadedMainsAssignment = false;
    bool m_loadedBatteryAssignment = false;
    bool m_saveInProgress = false;
  };
}
