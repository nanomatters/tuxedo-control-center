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

#include "SystemMonitor.hpp"

namespace ucc
{

HardwareTab::HardwareTab( SystemMonitor *systemMonitor, QWidget *parent )
  : QWidget( parent )
  , m_systemMonitor( systemMonitor )
{
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

}