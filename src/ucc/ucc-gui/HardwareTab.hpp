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
#include <QSlider>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QMainWindow>
#include <QStatusBar>
#include <QComboBox>
#include <QListWidget>
#include <QProgressBar>
#include <QMessageBox>

#include "LCTWaterCoolerController.hpp"

namespace ucc
{

class SystemMonitor;
class LCTWaterCoolerController;

/**
 * @brief Hardware tab widget for hardware controls
 */
class HardwareTab : public QWidget
{
    Q_OBJECT

public:
    explicit HardwareTab( SystemMonitor *systemMonitor, QWidget *parent = nullptr );
    ~HardwareTab() override = default;

    LCTWaterCoolerController* getWaterCoolerController() const { return m_waterCoolerController; }

private slots:
    void onDisplayBrightnessSliderChanged( int value );
    void onWebcamToggled( bool checked );
    void onFnLockToggled( bool checked );

    // Water cooler slots
    void onScanDevicesClicked();
    void onDisconnectDeviceClicked();
    void onDeviceDiscovered(const DeviceInfo &device);
    void onDiscoveryFinished();
    void onConnected();
    void onDisconnected();
    void onConnectionError(const QString &error);
    void onControlError(const QString &error);
    void onFanSpeedChanged(int value);
    void onPumpVoltageChanged(int index);
    void onLEDOnOffChanged(bool enabled);
    void onLEDModeChanged(int index);
    void onColorPickerClicked();

private:
    void setupUI();
    void connectSignals();

    SystemMonitor *m_systemMonitor;
    LCTWaterCoolerController *m_waterCoolerController = nullptr;

    // Display controls
    QSlider *m_displayBrightnessSlider = nullptr;
    QLabel *m_displayBrightnessValueLabel = nullptr;

    // Quick controls
    QCheckBox *m_webcamCheckBox = nullptr;
    QCheckBox *m_fnLockCheckBox = nullptr;

    // Water cooler controls
    QPushButton *m_scanButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QLabel *m_connectionStatusLabel = nullptr;
    QProgressBar *m_scanProgressBar = nullptr;

    // Fan controls
    QSlider *m_fanSpeedSlider = nullptr;
    QLabel *m_fanSpeedValueLabel = nullptr;
    QPushButton *m_fanOffButton = nullptr;

    // Pump controls
    QComboBox *m_pumpVoltageCombo = nullptr;

    // LED controls
    QCheckBox *m_ledOnOffCheckBox = nullptr;
    QPushButton *m_colorPickerButton = nullptr;
    QComboBox *m_ledModeCombo = nullptr;

    // Current LED color
    int m_currentRed = 255;
    int m_currentGreen = 0;
    int m_currentBlue = 0;
};

}