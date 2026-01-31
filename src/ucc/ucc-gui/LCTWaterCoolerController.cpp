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

#include "LCTWaterCoolerController.hpp"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QTimer>
#include <QDebug>
#include <iostream>

namespace ucc
{

// Static UUID constants
const QString LCTWaterCoolerController::NORDIC_UART_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const QString LCTWaterCoolerController::NORDIC_UART_CHAR_TX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const QString LCTWaterCoolerController::NORDIC_UART_CHAR_RX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

// Static QBluetoothUuid objects for proper comparison
const QBluetoothUuid LCTWaterCoolerController::NORDIC_UART_SERVICE_UUID_OBJ = QBluetoothUuid(NORDIC_UART_SERVICE_UUID);
const QBluetoothUuid LCTWaterCoolerController::NORDIC_UART_CHAR_TX_OBJ = QBluetoothUuid(NORDIC_UART_CHAR_TX);
const QBluetoothUuid LCTWaterCoolerController::NORDIC_UART_CHAR_RX_OBJ = QBluetoothUuid(NORDIC_UART_CHAR_RX);

LCTWaterCoolerController::LCTWaterCoolerController(QObject *parent)
    : QObject(parent)
{
    m_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &LCTWaterCoolerController::onDeviceDiscovered);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &LCTWaterCoolerController::onDiscoveryFinished);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, [this](QBluetoothDeviceDiscoveryAgent::Error error) {
        qDebug() << "Device discovery error:" << error;
        emit discoveryFinished();
    });
}

LCTWaterCoolerController::~LCTWaterCoolerController()
{
    if (m_bleController && m_bleController->state() != QLowEnergyController::UnconnectedState) {
        m_bleController->disconnectFromDevice();
    }
    stopDiscovery();
}

bool LCTWaterCoolerController::startDiscovery()
{
    if (m_isDiscovering) {
        return true;
    }

    m_discoveredDevices.clear();
    m_deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    m_isDiscovering = true;
    qDebug() << "Started Bluetooth device discovery";
    return true;
}

void LCTWaterCoolerController::stopDiscovery()
{
    if (m_isDiscovering) {
        m_deviceDiscoveryAgent->stop();
        m_isDiscovering = false;
        qDebug() << "Stopped Bluetooth device discovery";
    }
}

bool LCTWaterCoolerController::isDiscovering() const
{
    return m_isDiscovering;
}

QList<DeviceInfo> LCTWaterCoolerController::getDeviceList() const
{
    return m_discoveredDevices;
}

bool LCTWaterCoolerController::connectToDevice(const QString &deviceUuid)
{
    std::cout << "[DEBUG] LCTWaterCoolerController::connectToDevice called with UUID: " << deviceUuid.toStdString() << std::endl;

    if (m_isConnected) {
        std::cout << "[DEBUG] Already connected to a device" << std::endl;
        qDebug() << "Already connected to a device";
        return false;
    }

    // Find the device in discovered devices
    QBluetoothDeviceInfo deviceInfo;
    bool found = false;
    std::cout << "[DEBUG] Searching for device in " << m_discoveredDevices.size() << " discovered devices" << std::endl;
    for (const auto &device : m_discoveredDevices) {
        std::cout << "[DEBUG] Checking device: " << device.uuid.toStdString() << " name: " << device.name.toStdString() << std::endl;
        if (device.uuid == deviceUuid) {
            deviceInfo = device.deviceInfo;
            found = true;
            std::cout << "[DEBUG] Found matching device: " << device.name.toStdString() << std::endl;
            break;
        }
    }

    if (!found) {
        std::cout << "[DEBUG] Device not found in discovered devices: " << deviceUuid.toStdString() << std::endl;
        qDebug() << "Device not found in discovered devices:" << deviceUuid;
        emit connectionError("Device not found");
        return false;
    }

    m_connectedDeviceInfo = deviceInfo;
    std::cout << "[DEBUG] Creating BLE controller for device: " << deviceInfo.name().toStdString() << std::endl;
    m_bleController = QLowEnergyController::createCentral(deviceInfo, this);
    connect(m_bleController, &QLowEnergyController::connected, this, &LCTWaterCoolerController::onConnected);
    connect(m_bleController, &QLowEnergyController::disconnected, this, &LCTWaterCoolerController::onDisconnected);
    connect(m_bleController, &QLowEnergyController::errorOccurred, this, &LCTWaterCoolerController::onError);
    connect(m_bleController, &QLowEnergyController::serviceDiscovered,
            this, [this](const QBluetoothUuid &serviceUuid) {
        std::cout << "[DEBUG] Service discovered: " << serviceUuid.toString().toStdString() << std::endl;
        if (serviceUuid == NORDIC_UART_SERVICE_UUID_OBJ) {
            std::cout << "[DEBUG] Found Nordic UART service during discovery" << std::endl;
            qDebug() << "Found Nordic UART service";
        }
    });
    connect(m_bleController, &QLowEnergyController::discoveryFinished,
            this, &LCTWaterCoolerController::onServiceDiscoveryFinished);

    std::cout << "[DEBUG] Calling connectToDevice()" << std::endl;
    m_bleController->connectToDevice();
    qDebug() << "Connecting to device:" << deviceUuid;
    return true;
}

void LCTWaterCoolerController::disconnectFromDevice()
{
    if (m_bleController && m_isConnected) {
        // Send reset command before disconnecting
        writeCommand(QByteArray::fromHex("fe190001000000ef"));
        QTimer::singleShot(500, this, [this]() {
            m_bleController->disconnectFromDevice();
        });
    }
}

bool LCTWaterCoolerController::isConnected() const
{
    return m_isConnected;
}

LCTDeviceModel LCTWaterCoolerController::getConnectedModel() const
{
    return m_connectedModel;
}

bool LCTWaterCoolerController::setFanSpeed(int dutyCyclePercent)
{
    if (!m_isConnected || dutyCyclePercent < 0 || dutyCyclePercent > 100) {
        return false;
    }

    QByteArray data;
    data.append(static_cast<char>(0xfe));
    data.append(static_cast<char>(CMD_FAN));
    data.append(static_cast<char>(0x01));  // Enable
    data.append(static_cast<char>(dutyCyclePercent));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0xef));

    return writeCommand(data);
}

bool LCTWaterCoolerController::setPumpVoltage(PumpVoltage voltage)
{
    if (!m_isConnected) {
        return false;
    }

    QByteArray data;
    data.append(static_cast<char>(0xfe));
    data.append(static_cast<char>(CMD_PUMP));
    data.append(static_cast<char>(0x01));  // Enable
    data.append(static_cast<char>(60));  // Fixed duty cycle
    data.append(static_cast<char>(static_cast<uint8_t>(voltage)));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0xef));

    return writeCommand(data);
}

bool LCTWaterCoolerController::turnOffFan()
{
    if (!m_isConnected) {
        return false;
    }

    QByteArray data;
    data.append(static_cast<char>(0xfe));
    data.append(static_cast<char>(CMD_FAN));
    data.append(static_cast<char>(0x00));  // Disable
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0xef));

    return writeCommand(data);
}

bool LCTWaterCoolerController::turnOffPump()
{
    if (!m_isConnected) {
        return false;
    }

    QByteArray data;
    data.append(static_cast<char>(0xfe));
    data.append(static_cast<char>(CMD_PUMP));
    data.append(static_cast<char>(0x00));  // Disable
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0xef));

    return writeCommand(data);
}

bool LCTWaterCoolerController::setLEDColor(int red, int green, int blue, RGBState mode)
{
    if (!m_isConnected || red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255) {
        return false;
    }

    QByteArray data;
    data.append(static_cast<char>(0xfe));
    data.append(static_cast<char>(CMD_RGB));
    data.append(static_cast<char>(0x01));  // Enable
    data.append(static_cast<char>(red));
    data.append(static_cast<char>(green));
    data.append(static_cast<char>(blue));
    data.append(static_cast<char>(static_cast<uint8_t>(mode)));
    data.append(static_cast<char>(0xef));

    return writeCommand(data);
}

bool LCTWaterCoolerController::turnOffLED()
{
    if (!m_isConnected) {
        return false;
    }

    QByteArray data;
    data.append(static_cast<char>(0xfe));
    data.append(static_cast<char>(CMD_RGB));
    data.append(static_cast<char>(0x00));  // Disable
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0xef));

    return writeCommand(data);
}

void LCTWaterCoolerController::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    std::cout << "[DEBUG] onDeviceDiscovered() called for device: " << device.name().toStdString() 
              << " address: " << device.address().toString().toStdString() << std::endl;

    // Check if this is an LCT device
    QString deviceName = device.name();
    LCTDeviceModel model = deviceModelFromName(deviceName);
    std::cout << "[DEBUG] Device model detected: " << static_cast<int>(model) << std::endl;

    if (model == LCTDeviceModel::Unknown) {
        std::cout << "[DEBUG] Not an LCT device, ignoring" << std::endl;
        return;  // Not an LCT device
    }

    DeviceInfo info;
    info.uuid = device.address().toString();
    info.name = deviceName;
    info.rssi = device.rssi();
    info.deviceInfo = device;

    std::cout << "[DEBUG] Adding LCT device to discovered list: " << deviceName.toStdString() 
              << " UUID: " << info.uuid.toStdString() << std::endl;
    m_discoveredDevices.append(info);
    emit deviceDiscovered(info);
    qDebug() << "Discovered LCT device:" << deviceName << "UUID:" << info.uuid;
}

void LCTWaterCoolerController::onDiscoveryFinished()
{
    std::cout << "[DEBUG] onDiscoveryFinished() called" << std::endl;
    m_isDiscovering = false;
    std::cout << "[DEBUG] Device discovery finished, found " << m_discoveredDevices.size() << " LCT devices" << std::endl;
    emit discoveryFinished();
    qDebug() << "Device discovery finished, found" << m_discoveredDevices.size() << "LCT devices";
}

void LCTWaterCoolerController::onConnected()
{
    std::cout << "[DEBUG] onConnected() called - successfully connected to BLE device" << std::endl;
    qDebug() << "Connected to device, starting service discovery";
    std::cout << "[DEBUG] Calling discoverServices()" << std::endl;
    m_bleController->discoverServices();
}

void LCTWaterCoolerController::onDisconnected()
{
    m_isConnected = false;
    m_connectedModel = LCTDeviceModel::LCT21001;  // Reset to default
    emit disconnected();
    qDebug() << "Disconnected from device";
}

void LCTWaterCoolerController::onServiceDiscoveryFinished()
{
    std::cout << "[DEBUG] onServiceDiscoveryFinished() called" << std::endl;
    qDebug() << "Service discovery finished";

    // Debug: List all discovered services
    auto services = m_bleController->services();
    std::cout << "[DEBUG] Number of discovered services: " << services.size() << std::endl;
    qDebug() << "Discovered services:";
    for (const auto &service : services) {
        std::cout << "[DEBUG] Service UUID: " << service.toString().toStdString() << std::endl;
        qDebug() << "  Service UUID:" << service.toString();
    }

    // Find the Nordic UART service
    std::cout << "[DEBUG] Looking for Nordic UART service: " << NORDIC_UART_SERVICE_UUID_OBJ.toString().toStdString() << std::endl;
    for (const auto &service : services) {
        std::cout << "[DEBUG] Checking service: " << service.toString().toStdString() << std::endl;
        if (service == NORDIC_UART_SERVICE_UUID_OBJ) {
            std::cout << "[DEBUG] Found Nordic UART service, creating service object" << std::endl;
            qDebug() << "Found Nordic UART service, creating service object";
            m_uartService = m_bleController->createServiceObject(service, this);
            if (m_uartService) {
                std::cout << "[DEBUG] Service object created successfully" << std::endl;
                connect(m_uartService, &QLowEnergyService::characteristicRead,
                        this, &LCTWaterCoolerController::onCharacteristicRead);
                connect(m_uartService, &QLowEnergyService::characteristicWritten,
                        this, &LCTWaterCoolerController::onCharacteristicWritten);
                connect(m_uartService, &QLowEnergyService::stateChanged,
                        this, &LCTWaterCoolerController::onServiceStateChanged);
                connect(m_uartService, &QLowEnergyService::errorOccurred,
                        this, [this](QLowEnergyService::ServiceError error) {
                    std::cout << "[DEBUG] Service error occurred: " << static_cast<int>(error) << std::endl;
                    qDebug() << "Service error:" << error;
                });

                // Check if service is already discovered
                auto serviceState = m_uartService->state();
                std::cout << "[DEBUG] Service state: " << static_cast<int>(serviceState) << std::endl;
                if (serviceState == QLowEnergyService::RemoteServiceDiscovered) {
                    std::cout << "[DEBUG] Service already discovered, checking characteristics directly" << std::endl;
                    qDebug() << "Service already discovered, checking characteristics directly";
                    onServiceStateChanged(QLowEnergyService::RemoteServiceDiscovered);
                } else {
                    std::cout << "[DEBUG] Service not discovered yet, calling discoverDetails()" << std::endl;
                    qDebug() << "Service not discovered yet, calling discoverDetails()";
                    m_uartService->discoverDetails();
                }
                return; // Wait for service discovery to complete
            } else {
                std::cout << "[DEBUG] Failed to create service object" << std::endl;
                qDebug() << "Failed to create service object";
            }
        }
    }

    std::cout << "[DEBUG] Nordic UART service not found among discovered services" << std::endl;
    qDebug() << "Nordic UART service not found among discovered services";
    // If we get here, connection failed
    emit connectionError("Failed to find Nordic UART service or characteristics");
    m_bleController->disconnectFromDevice();
}

void LCTWaterCoolerController::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    std::cout << "[DEBUG] onServiceStateChanged() called with state: " << static_cast<int>(state) << std::endl;
    qDebug() << "Service state changed:" << state;

    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        std::cout << "[DEBUG] Service discovery completed, checking for characteristics" << std::endl;
        qDebug() << "Service discovery completed, checking for characteristics";

        // Debug: List all characteristics
        auto characteristics = m_uartService->characteristics();
        std::cout << "[DEBUG] Number of characteristics: " << characteristics.size() << std::endl;
        qDebug() << "Service characteristics:";
        for (const auto &characteristic : characteristics) {
            std::cout << "[DEBUG] Characteristic UUID: " << characteristic.uuid().toString().toStdString() << std::endl;
            qDebug() << "  Characteristic UUID:" << characteristic.uuid().toString();
        }

        // Service discovery completed, now check for characteristics
        std::cout << "[DEBUG] Looking for TX characteristic: " << NORDIC_UART_CHAR_TX_OBJ.toString().toStdString() << std::endl;
        std::cout << "[DEBUG] Looking for RX characteristic: " << NORDIC_UART_CHAR_RX_OBJ.toString().toStdString() << std::endl;
        for (const auto &characteristic : characteristics) {
            std::string charUuid = characteristic.uuid().toString().toStdString();
            if (characteristic.uuid() == NORDIC_UART_CHAR_TX_OBJ) {
                m_txCharacteristic = characteristic;
                std::cout << "[DEBUG] Found TX characteristic" << std::endl;
                qDebug() << "Found TX characteristic";
            } else if (characteristic.uuid() == NORDIC_UART_CHAR_RX_OBJ) {
                m_rxCharacteristic = characteristic;
                std::cout << "[DEBUG] Found RX characteristic" << std::endl;
                qDebug() << "Found RX characteristic";
            }
        }

        bool txValid = m_txCharacteristic.isValid();
        bool rxValid = m_rxCharacteristic.isValid();
        std::cout << "[DEBUG] TX characteristic valid: " << (txValid ? "YES" : "NO") << std::endl;
        std::cout << "[DEBUG] RX characteristic valid: " << (rxValid ? "YES" : "NO") << std::endl;

        if (txValid && rxValid) {
            m_isConnected = true;
            // Determine device model from device name
            QString deviceName = m_connectedDeviceInfo.name();
            m_connectedModel = deviceModelFromName(deviceName);
            std::cout << "[DEBUG] Successfully connected to LCT device: " << deviceName.toStdString() << std::endl;
            emit connected();
            qDebug() << "Successfully connected to LCT device:" << deviceName;
        } else {
            std::cout << "[DEBUG] Characteristics not found - connection failed" << std::endl;
            qDebug() << "TX characteristic valid:" << m_txCharacteristic.isValid();
            qDebug() << "RX characteristic valid:" << m_rxCharacteristic.isValid();
            emit connectionError("Failed to find Nordic UART TX/RX characteristics");
            m_bleController->disconnectFromDevice();
        }
    }
}

void LCTWaterCoolerController::onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    Q_UNUSED(characteristic)
    Q_UNUSED(value)
    // Handle responses if needed
}

void LCTWaterCoolerController::onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    Q_UNUSED(characteristic)
    Q_UNUSED(value)
    // Command sent successfully
}

void LCTWaterCoolerController::onError(QLowEnergyController::Error error)
{
    std::cout << "[DEBUG] onError() called with error code: " << static_cast<int>(error) << std::endl;
    QString errorMsg;
    switch (error) {
    case QLowEnergyController::ConnectionError:
        errorMsg = "Connection error";
        std::cout << "[DEBUG] BLE Connection Error" << std::endl;
        break;
    case QLowEnergyController::AdvertisingError:
        errorMsg = "Advertising error";
        std::cout << "[DEBUG] BLE Advertising Error" << std::endl;
        break;
    default:
        errorMsg = "Unknown error";
        std::cout << "[DEBUG] BLE Unknown Error" << std::endl;
        break;
    }

    std::cout << "[DEBUG] BLE Controller error: " << errorMsg.toStdString() << std::endl;
    qDebug() << "BLE Controller error:" << errorMsg;
    emit connectionError(errorMsg);
    m_isConnected = false;
}

LCTDeviceModel LCTWaterCoolerController::deviceModelFromName(const QString &name) const
{
    QString lowerName = name.toLower();
    if (lowerName.contains("lct22002")) {
        return LCTDeviceModel::LCT22002;
    } else if (lowerName.contains("lct21001")) {
        return LCTDeviceModel::LCT21001;
    }
    return LCTDeviceModel::Unknown;  // Not an LCT device
}

bool LCTWaterCoolerController::writeCommand(const QByteArray &data)
{
    if (!m_isConnected || !m_txCharacteristic.isValid()) {
        qDebug() << "Not connected or TX characteristic not available";
        emit controlError("Not connected to device");
        return false;
    }

    m_uartService->writeCharacteristic(m_txCharacteristic, data, QLowEnergyService::WriteWithoutResponse);
    qDebug() << "Sent command:" << data.toHex();
    return true;
}

bool LCTWaterCoolerController::writeReceive(const QByteArray &data)
{
    if (!m_isConnected || !m_txCharacteristic.isValid() || !m_rxCharacteristic.isValid()) {
        qDebug() << "Not connected or characteristics not available";
        emit controlError("Not connected to device");
        return false;
    }

    // For write-receive operations, we would need to set up notifications on RX
    // and wait for response. For now, just write the command.
    return writeCommand(data);
}

}