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

#include "HardwareTab.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QProgressBar>
#include <QColorDialog>

#include "SystemMonitor.hpp"
#include "LCTWaterCoolerController.hpp"

namespace ucc
{

HardwareTab::HardwareTab( SystemMonitor *systemMonitor, QWidget *parent )
  : QWidget( parent )
  , m_systemMonitor( systemMonitor )
{
  m_waterCoolerController = new LCTWaterCoolerController(this);
  setupUI();
  connectSignals();
}

void HardwareTab::setupUI()
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setContentsMargins( 20, 20, 20, 20 );
  layout->setSpacing( 16 );

  QLabel *titleLabel = new QLabel( "Hardware & Controls" );
  titleLabel->setStyleSheet( "font-size: 24px; font-weight: bold;" );
  layout->addWidget( titleLabel );

  // Display Controls Group
  QGroupBox *displayGroup = new QGroupBox( "Display" );
  QVBoxLayout *displayLayout = new QVBoxLayout( displayGroup );

  QHBoxLayout *brightnessLayout = new QHBoxLayout();
  QLabel *brightnessLabel = new QLabel( "Brightness:" );
  brightnessLabel->setStyleSheet( "font-weight: bold;" );
  m_displayBrightnessSlider = new QSlider( Qt::Horizontal );
  m_displayBrightnessSlider->setMinimum( 0 );
  m_displayBrightnessSlider->setMaximum( 100 );
  m_displayBrightnessSlider->setValue( 50 );
  m_displayBrightnessValueLabel = new QLabel( "50%" );
  m_displayBrightnessValueLabel->setMinimumWidth( 40 );
  brightnessLayout->addWidget( brightnessLabel );
  brightnessLayout->addWidget( m_displayBrightnessSlider );
  brightnessLayout->addWidget( m_displayBrightnessValueLabel );
  displayLayout->addLayout( brightnessLayout );

  layout->addWidget( displayGroup );

  // Quick Controls Group
  QGroupBox *controlsGroup = new QGroupBox( "Quick Controls" );
  QVBoxLayout *controlsLayout = new QVBoxLayout( controlsGroup );

  m_webcamCheckBox = new QCheckBox( "Webcam Enabled" );
  m_fnLockCheckBox = new QCheckBox( "Fn Lock Enabled" );
  controlsLayout->addWidget( m_webcamCheckBox );
  controlsLayout->addWidget( m_fnLockCheckBox );

  layout->addWidget( controlsGroup );

  // Water Cooler Controls Group
  QGroupBox *waterCoolerGroup = new QGroupBox( "Water Cooler (LCT22001/LCT22002)" );
  QVBoxLayout *waterCoolerLayout = new QVBoxLayout( waterCoolerGroup );

  // Device discovery section
  QHBoxLayout *discoveryLayout = new QHBoxLayout();
  m_scanButton = new QPushButton( "Scan for Devices" );
  m_scanProgressBar = new QProgressBar();
  m_scanProgressBar->setRange( 0, 0 );  // Indeterminate progress
  m_scanProgressBar->setVisible( false );
  discoveryLayout->addWidget( m_scanButton );
  discoveryLayout->addWidget( m_scanProgressBar );
  waterCoolerLayout->addLayout( discoveryLayout );

  // Device list
  QLabel *deviceListLabel = new QLabel( "Available Devices:" );
  deviceListLabel->setStyleSheet( "font-weight: bold;" );
  waterCoolerLayout->addWidget( deviceListLabel );

  m_deviceListWidget = new QListWidget();
  m_deviceListWidget->setMaximumHeight( 100 );
  waterCoolerLayout->addWidget( m_deviceListWidget );

  // Connection controls
  QHBoxLayout *connectionLayout = new QHBoxLayout();
  m_connectButton = new QPushButton( "Connect" );
  m_disconnectButton = new QPushButton( "Disconnect" );
  m_connectionStatusLabel = new QLabel( "Not connected" );
  m_connectionStatusLabel->setStyleSheet( "color: red;" );

  connectionLayout->addWidget( m_connectButton );
  connectionLayout->addWidget( m_disconnectButton );
  connectionLayout->addStretch();
  connectionLayout->addWidget( m_connectionStatusLabel );
  waterCoolerLayout->addLayout( connectionLayout );

  // Fan controls
  QHBoxLayout *fanSpeedLayout = new QHBoxLayout();
  QLabel *fanSpeedLabel = new QLabel( "Fan Speed:" );
  fanSpeedLabel->setStyleSheet( "font-weight: bold;" );
  m_fanSpeedSlider = new QSlider( Qt::Horizontal );
  m_fanSpeedSlider->setMinimum( 0 );
  m_fanSpeedSlider->setMaximum( 100 );
  m_fanSpeedSlider->setValue( 50 );
  m_fanSpeedSlider->setEnabled( false );
  m_fanSpeedValueLabel = new QLabel( "50%" );
  m_fanSpeedValueLabel->setMinimumWidth( 40 );
  m_fanOffButton = new QPushButton( "Turn Off" );
  m_fanOffButton->setEnabled( false );

  fanSpeedLayout->addWidget( fanSpeedLabel );
  fanSpeedLayout->addWidget( m_fanSpeedSlider );
  fanSpeedLayout->addWidget( m_fanSpeedValueLabel );
  fanSpeedLayout->addWidget( m_fanOffButton );
  waterCoolerLayout->addLayout( fanSpeedLayout );

  // Pump controls
  QHBoxLayout *pumpVoltageLayout = new QHBoxLayout();
  QLabel *pumpVoltageLabel = new QLabel( "Pump Voltage:" );
  pumpVoltageLabel->setStyleSheet( "font-weight: bold;" );
  m_pumpVoltageCombo = new QComboBox();
  m_pumpVoltageCombo->addItem( "7V", QVariant::fromValue( PumpVoltage::V7 ) );
  m_pumpVoltageCombo->addItem( "8V", QVariant::fromValue( PumpVoltage::V8 ) );
  m_pumpVoltageCombo->addItem( "11V", QVariant::fromValue( PumpVoltage::V11 ) );
  m_pumpVoltageCombo->addItem( "12V", QVariant::fromValue( PumpVoltage::V12 ) );
  m_pumpVoltageCombo->addItem( "Off", QVariant::fromValue( PumpVoltage::Off ) );
  m_pumpVoltageCombo->setCurrentIndex( 2 );  // Default to 8V (index 2)
  m_pumpVoltageCombo->setEnabled( false );

  pumpVoltageLayout->addWidget( pumpVoltageLabel );
  pumpVoltageLayout->addWidget( m_pumpVoltageCombo );
  pumpVoltageLayout->addStretch();
  waterCoolerLayout->addLayout( pumpVoltageLayout );

  // LED controls
  QLabel *ledLabel = new QLabel( "LED:" );
  ledLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
  waterCoolerLayout->addWidget( ledLabel );

  // LED enable/disable
  QHBoxLayout *ledEnableLayout = new QHBoxLayout();
  m_ledOnOffCheckBox = new QCheckBox( "Enabled" );
  m_ledOnOffCheckBox->setChecked( true );
  m_ledOnOffCheckBox->setEnabled( false );
  ledEnableLayout->addWidget( m_ledOnOffCheckBox );
  ledEnableLayout->addStretch();
  waterCoolerLayout->addLayout( ledEnableLayout );

  // Color controls
  QHBoxLayout *colorLayout = new QHBoxLayout();
  QLabel *colorLabel = new QLabel( "Color:" );
  colorLabel->setStyleSheet( "font-weight: bold;" );
  m_colorPickerButton = new QPushButton( "Choose Color" );
  m_colorPickerButton->setEnabled( false );
  colorLayout->addWidget( colorLabel );
  colorLayout->addWidget( m_colorPickerButton );
  colorLayout->addStretch();
  waterCoolerLayout->addLayout( colorLayout );

  // LED mode
  QHBoxLayout *ledModeLayout = new QHBoxLayout();
  QLabel *ledModeLabel = new QLabel( "Mode:" );
  ledModeLabel->setStyleSheet( "font-weight: bold;" );
  m_ledModeCombo = new QComboBox();
  m_ledModeCombo->addItem( "Static", QVariant::fromValue( RGBState::Static ) );
  m_ledModeCombo->addItem( "Breathe", QVariant::fromValue( RGBState::Breathe ) );
  m_ledModeCombo->addItem( "Colorful", QVariant::fromValue( RGBState::Colorful ) );
  m_ledModeCombo->addItem( "Breathe Color", QVariant::fromValue( RGBState::BreatheColor ) );
  m_ledModeCombo->setCurrentIndex( 0 );  // Default to Static
  m_ledModeCombo->setEnabled( false );

  ledModeLayout->addWidget( ledModeLabel );
  ledModeLayout->addWidget( m_ledModeCombo );
  ledModeLayout->addStretch();
  waterCoolerLayout->addLayout( ledModeLayout );

  layout->addWidget( waterCoolerGroup );

  layout->addStretch();
}

void HardwareTab::connectSignals()
{
  connect( m_displayBrightnessSlider, &QSlider::sliderMoved,
           this, &HardwareTab::onDisplayBrightnessSliderChanged );

  connect( m_displayBrightnessSlider, &QSlider::valueChanged,
           [this]( int value ) {
    if ( m_displayBrightnessValueLabel )
      m_displayBrightnessValueLabel->setText( QString::number( value ) + "%" );
  } );

  connect( m_webcamCheckBox, &QCheckBox::toggled,
           this, &HardwareTab::onWebcamToggled );

  connect( m_fnLockCheckBox, &QCheckBox::toggled,
           this, &HardwareTab::onFnLockToggled );

  // Water cooler connections
  connect( m_scanButton, &QPushButton::clicked,
           this, &HardwareTab::onScanDevicesClicked );
  connect( m_connectButton, &QPushButton::clicked,
           this, &HardwareTab::onConnectDeviceClicked );
  connect( m_disconnectButton, &QPushButton::clicked,
           this, &HardwareTab::onDisconnectDeviceClicked );

  connect( m_waterCoolerController, &LCTWaterCoolerController::deviceDiscovered,
           this, &HardwareTab::onDeviceDiscovered );
  connect( m_waterCoolerController, &LCTWaterCoolerController::discoveryFinished,
           this, &HardwareTab::onDiscoveryFinished );
  connect( m_waterCoolerController, &LCTWaterCoolerController::connected,
           this, &HardwareTab::onConnected );
  connect( m_waterCoolerController, &LCTWaterCoolerController::disconnected,
           this, &HardwareTab::onDisconnected );
  connect( m_waterCoolerController, &LCTWaterCoolerController::connectionError,
           this, &HardwareTab::onConnectionError );
  connect( m_waterCoolerController, &LCTWaterCoolerController::controlError,
           this, &HardwareTab::onControlError );

  connect( m_fanSpeedSlider, &QSlider::valueChanged,
           this, &HardwareTab::onFanSpeedChanged );
  connect( m_fanOffButton, &QPushButton::clicked,
           this, [this]() { m_waterCoolerController->turnOffFan(); } );

  connect( m_pumpVoltageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
           this, &HardwareTab::onPumpVoltageChanged );

  connect( m_ledOnOffCheckBox, &QCheckBox::toggled,
           this, &HardwareTab::onLEDOnOffChanged );
  connect( m_colorPickerButton, &QPushButton::clicked,
           this, &HardwareTab::onColorPickerClicked );
  connect( m_ledModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
           this, &HardwareTab::onLEDModeChanged );
}

void HardwareTab::onDisplayBrightnessSliderChanged( int value )
{
  if ( m_systemMonitor )
  {
    m_systemMonitor->setDisplayBrightness( value );
  }
}

void HardwareTab::onWebcamToggled( bool checked )
{
  if ( m_systemMonitor )
  {
    m_systemMonitor->setWebcamEnabled( checked );
  }
}

void HardwareTab::onFnLockToggled( bool checked )
{
  if ( m_systemMonitor )
  {
    m_systemMonitor->setFnLock( checked );
  }
}

void HardwareTab::onScanDevicesClicked()
{
  m_deviceListWidget->clear();
  m_scanProgressBar->setVisible( true );
  m_scanProgressBar->setRange( 0, 0 ); // Indeterminate progress
  m_scanButton->setEnabled( false );
  m_waterCoolerController->startDiscovery();
}

void HardwareTab::onConnectDeviceClicked()
{
  auto currentItem = m_deviceListWidget->currentItem();
  if ( !currentItem ) {
    QMessageBox::warning( this, tr( "No Device Selected" ),
                         tr( "Please select a water cooler device to connect to." ) );
    return;
  }

  QString deviceAddress = currentItem->data( Qt::UserRole ).toString();
  m_waterCoolerController->connectToDevice( deviceAddress );
  m_connectButton->setEnabled( false );
  m_disconnectButton->setEnabled( true );
}

void HardwareTab::onDisconnectDeviceClicked()
{
  m_waterCoolerController->disconnectFromDevice();
}

void HardwareTab::onDeviceDiscovered( const DeviceInfo& device )
{
  // Filter for known water cooler models
  if ( device.name.contains( "LCT", Qt::CaseInsensitive ) ||
       device.name.contains( "22001", Qt::CaseInsensitive ) ||
       device.name.contains( "22002", Qt::CaseInsensitive ) ) {
    auto item = new QListWidgetItem( device.name + " (" + device.uuid + ")" );
    item->setData( Qt::UserRole, device.uuid );
    m_deviceListWidget->addItem( item );
  }
}

void HardwareTab::onDiscoveryFinished()
{
  m_scanProgressBar->setVisible( false );
  m_scanButton->setEnabled( true );
  if ( m_deviceListWidget->count() == 0 ) {
    QMessageBox::information( this, tr( "No Devices Found" ),
                             tr( "No compatible water cooler devices were found. Make sure Bluetooth is enabled and the device is in pairing mode." ) );
  }
}

void HardwareTab::onConnected()
{
  m_connectButton->setEnabled( false );
  m_disconnectButton->setEnabled( true );
  m_fanSpeedSlider->setEnabled( true );
  m_fanOffButton->setEnabled( true );
  m_pumpVoltageCombo->setEnabled( true );
  m_ledOnOffCheckBox->setEnabled( true );
  m_colorPickerButton->setEnabled( true );
  m_ledModeCombo->setEnabled( true );

  QMessageBox::information( this, tr( "Connected" ),
                           tr( "Successfully connected to water cooler device." ) );
}

void HardwareTab::onDisconnected()
{
  m_connectButton->setEnabled( true );
  m_disconnectButton->setEnabled( false );
  m_fanSpeedSlider->setEnabled( false );
  m_fanOffButton->setEnabled( false );
  m_pumpVoltageCombo->setEnabled( false );
  m_ledOnOffCheckBox->setEnabled( false );
  m_colorPickerButton->setEnabled( false );
  m_ledModeCombo->setEnabled( false );

  QMessageBox::information( this, tr( "Disconnected" ),
                           tr( "Disconnected from water cooler device." ) );
}

void HardwareTab::onConnectionError( const QString& error )
{
  m_connectButton->setEnabled( true );
  m_disconnectButton->setEnabled( false );
  QMessageBox::critical( this, tr( "Connection Error" ), error );
}

void HardwareTab::onControlError( const QString& error )
{
  QMessageBox::warning( this, tr( "Control Error" ), error );
}

void HardwareTab::onFanSpeedChanged( int value )
{
  if ( m_waterCoolerController->isConnected() ) {
    m_waterCoolerController->setFanSpeed( value );
  }
}

void HardwareTab::onPumpVoltageChanged( int index )
{
  if ( m_waterCoolerController->isConnected() ) {
    if ( index == static_cast< int >( PumpVoltage::Off ) )
    {  // Off
      m_waterCoolerController->turnOffPump();
    } else
    {
      PumpVoltage voltage = static_cast<PumpVoltage>( m_pumpVoltageCombo->itemData( index ).toInt() );
      m_waterCoolerController->setPumpVoltage( voltage );
    }
  }
}

void HardwareTab::onLEDOnOffChanged( bool enabled )
{
  if ( m_waterCoolerController->isConnected() ) {
    if ( enabled ) {
      RGBState mode = static_cast<RGBState>(m_ledModeCombo->currentData().toInt());
      m_waterCoolerController->setLEDColor( m_currentRed, m_currentGreen, m_currentBlue, mode );
    } else {
      m_waterCoolerController->turnOffLED();
    }
  }
}

void HardwareTab::onLEDModeChanged( int /*index*/ )
{
  if ( m_waterCoolerController->isConnected() && m_ledOnOffCheckBox->isChecked() ) {
    RGBState mode = static_cast<RGBState>(m_ledModeCombo->currentData().toInt());
    m_waterCoolerController->setLEDColor( m_currentRed, m_currentGreen, m_currentBlue, mode );
  }
}

void HardwareTab::onColorPickerClicked()
{
  QColor currentColor( m_currentRed, m_currentGreen, m_currentBlue );
  QColor color = QColorDialog::getColor( currentColor, this, "Choose LED Color" );
  if ( color.isValid() ) {
    m_currentRed = color.red();
    m_currentGreen = color.green();
    m_currentBlue = color.blue();
    RGBState mode = static_cast<RGBState>(m_ledModeCombo->currentData().toInt());
    if ( m_waterCoolerController->isConnected() && m_ledOnOffCheckBox->isChecked() ) {
      m_waterCoolerController->setLEDColor( m_currentRed, m_currentGreen, m_currentBlue, mode );
    }
    // Update button color to show selected color
    QString style = QString( "background-color: rgb(%1, %2, %3);" ).arg( m_currentRed ).arg( m_currentGreen ).arg( m_currentBlue );
    m_colorPickerButton->setStyleSheet( style );
  }
}

}
