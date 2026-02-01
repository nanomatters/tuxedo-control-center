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
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QTimer>
#include <QList>

namespace ucc
{

enum class LCTDeviceModel {
    Unknown = -1,
    LCT21001 = 0,
    LCT22002 = 1
};

enum class PumpVoltage {
    V11 = 0x00,
    V12 = 0x01,
    V7 = 0x02,
    V8 = 0x03,
    Off = 0x04
};

enum class RGBState {
    Static = 0x00,
    Breathe = 0x01,
    Colorful = 0x02,
    BreatheColor = 0x03
};

struct DeviceInfo {
    QString uuid;
    QString name;
    int rssi;
    QBluetoothDeviceInfo deviceInfo;
};

class LCTWaterCoolerController : public QObject
{
    Q_OBJECT

public:
    explicit LCTWaterCoolerController(QObject *parent = nullptr);
    ~LCTWaterCoolerController() override;

    // Device discovery
    bool startDiscovery();
    void stopDiscovery();
    bool isDiscovering() const;
    QList<DeviceInfo> getDeviceList() const;

    // Connection management
    bool connectToDevice(const QString &deviceUuid);
    void disconnectFromDevice();
    bool isConnected() const;
    LCTDeviceModel getConnectedModel() const;

    // Control methods
    bool setFanSpeed(int dutyCyclePercent);  // 0-100
    bool setPumpVoltage(PumpVoltage voltage);
    bool setLEDColor(int red, int green, int blue, RGBState mode);  // 0-255, RGB state
    bool turnOffLED();
    bool turnOffFan();
    bool turnOffPump();

signals:
    void deviceDiscovered(const DeviceInfo &device);
    void discoveryFinished();
    void discoveryStarted();
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    void controlError(const QString &error);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void onDiscoveryFinished();
    void onConnected();
    void onDisconnected();
    void onServiceDiscoveryFinished();
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onError(QLowEnergyController::Error error);

private:
    // Bluetooth UUIDs (same as in LCT21001.ts)
    static const QString NORDIC_UART_SERVICE_UUID;
    static const QString NORDIC_UART_CHAR_TX;
    static const QString NORDIC_UART_CHAR_RX;
    static const QBluetoothUuid NORDIC_UART_SERVICE_UUID_OBJ;
    static const QBluetoothUuid NORDIC_UART_CHAR_TX_OBJ;
    static const QBluetoothUuid NORDIC_UART_CHAR_RX_OBJ;

    // Command constants
    static constexpr uint8_t CMD_RESET = 0x19;
    static constexpr uint8_t CMD_FAN = 0x1b;
    static constexpr uint8_t CMD_PUMP = 0x1c;
    static constexpr uint8_t CMD_RGB = 0x1e;

    LCTDeviceModel deviceModelFromName(const QString &name) const;
    bool writeCommand(const QByteArray &data);
    bool writeReceive(const QByteArray &data);

    QBluetoothDeviceDiscoveryAgent *m_deviceDiscoveryAgent = nullptr;
    QLowEnergyController *m_bleController = nullptr;
    QLowEnergyService *m_uartService = nullptr;
    QLowEnergyCharacteristic m_txCharacteristic;
    QLowEnergyCharacteristic m_rxCharacteristic;

    QBluetoothDeviceInfo m_connectedDeviceInfo;

    QList<DeviceInfo> m_discoveredDevices;
    LCTDeviceModel m_connectedModel;
    bool m_isConnected = false;
    bool m_isDiscovering = false;

    // Pending operations
    bool m_waitingForResponse = false;
    QByteArray m_pendingData;
};

}