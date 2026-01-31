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

#include "DashboardTab.hpp"
#include "SystemMonitor.hpp"
#include "ProfileManager.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>

namespace
{

QString formatFanSpeed( const QString &fanSpeed )
{
  QString display = "---";

  if ( fanSpeed.endsWith( " %" ) || fanSpeed.endsWith( "%" ) )
  {
    QString num = fanSpeed;
    if ( fanSpeed.endsWith( " %" ) )
      num = fanSpeed.left( fanSpeed.size() - 2 ).trimmed();
    else
      num = fanSpeed.left( fanSpeed.size() - 1 ).trimmed();

    bool ok = false;
    int pct = num.toInt( &ok );
    if ( ok && pct >= 0 )
      display = QString::number( pct );
  }
  else if ( fanSpeed.endsWith( " RPM" ) )
  {
    QString rpmStr = fanSpeed.left( fanSpeed.size() - 4 ).trimmed();
    bool ok = false;
    int rpm = rpmStr.toInt( &ok );

    if ( ok && rpm > 0 )
    {
      int pct = rpm / 60;
      display = QString::number( pct );
    }
  }

  return display;
}

}


namespace ucc
{

DashboardTab::DashboardTab( SystemMonitor *systemMonitor, ProfileManager *profileManager, QWidget *parent )
  : QWidget( parent )
  , m_systemMonitor( systemMonitor )
  , m_profileManager( profileManager )
{
  setupUI();
  connectSignals();
  
  // Initialize active profile label
  m_activeProfileLabel->setText( m_profileManager->activeProfile() );
}

void DashboardTab::setupUI()
{
  setStyleSheet( "background-color: #242424; color: #e6e6e6;" );
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setContentsMargins( 20, 20, 20, 20 );
  layout->setSpacing( 20 );

  // Title
  QLabel *titleLabel = new QLabel( "System monitor" );
  titleLabel->setStyleSheet( "font-size: 22px; font-weight: bold;" );
  titleLabel->setAlignment( Qt::AlignCenter );
  layout->addWidget( titleLabel );

  // Active Profile
  QHBoxLayout *activeLayout = new QHBoxLayout();
  QLabel *activeLabel = new QLabel( "Active profile:" );
  activeLabel->setStyleSheet( "font-weight: bold;" );
  m_activeProfileLabel = new QLabel( "Loading..." );
  m_activeProfileLabel->setStyleSheet( "color: #d32f2f; font-weight: bold;" );
  activeLayout->addStretch();
  activeLayout->addWidget( activeLabel );
  activeLayout->addSpacing( 6 );
  activeLayout->addWidget( m_activeProfileLabel );
  activeLayout->addStretch();
  layout->addLayout( activeLayout );

  auto makeGauge = [&]( const QString &caption, const QString &unit, QLabel *&valueLabel ) -> QWidget * {
    QWidget *container = new QWidget();
    QVBoxLayout *outer = new QVBoxLayout( container );
    outer->setContentsMargins( 0, 0, 0, 0 );
    outer->setSpacing( 8 );
    outer->setAlignment( Qt::AlignHCenter );

    QFrame *ring = new QFrame();
    ring->setFixedSize( 140, 140 );
    ring->setStyleSheet( "QFrame { border: 6px solid #b71c1c; border-radius: 70px; background: transparent; }" );

    QVBoxLayout *ringLayout = new QVBoxLayout( ring );
    ringLayout->setAlignment( Qt::AlignCenter );
    ringLayout->setSpacing( 2 );

    valueLabel = new QLabel( "--" );
    valueLabel->setStyleSheet( "font-size: 28px; font-weight: bold; color: #f5f5f5; background: transparent; border: none;" );
    valueLabel->setAlignment( Qt::AlignCenter );

    QLabel *unitLabel = new QLabel( unit );
    unitLabel->setStyleSheet( "font-size: 12px; color: #bdbdbd; background: transparent; border: none;" );
    unitLabel->setAlignment( Qt::AlignCenter );

    ringLayout->addWidget( valueLabel );
    ringLayout->addWidget( unitLabel );

    QLabel *captionLabel = new QLabel( caption );
    captionLabel->setStyleSheet( "font-size: 12px; color: #cfcfcf;" );
    captionLabel->setAlignment( Qt::AlignCenter );

    outer->addWidget( ring );
    outer->addWidget( captionLabel );
    return container;
  };

  // CPU section
  QLabel *cpuHeader = new QLabel( "Main processor monitor" );
  cpuHeader->setStyleSheet( "font-size: 14px; font-weight: bold;" );
  cpuHeader->setAlignment( Qt::AlignCenter );
  layout->addWidget( cpuHeader );

  QGridLayout *cpuGrid = new QGridLayout();
  cpuGrid->setHorizontalSpacing( 24 );
  cpuGrid->setVerticalSpacing( 16 );
  cpuGrid->addWidget( makeGauge( "CPU - Temp", "°C", m_cpuTempLabel ), 0, 0 );
  cpuGrid->addWidget( makeGauge( "CPU - Frequency", "GHz", m_cpuFrequencyLabel ), 0, 1 );
  cpuGrid->addWidget( makeGauge( "CPU - Fan", "%", m_fanSpeedLabel ), 0, 2 );
  cpuGrid->addWidget( makeGauge( "CPU - Power", "W", m_cpuPowerLabel ), 0, 3 );
  layout->addLayout( cpuGrid );

  // GPU section
  QLabel *gpuHeader = new QLabel( "Graphics card monitor" );
  gpuHeader->setStyleSheet( "font-size: 14px; font-weight: bold;" );
  gpuHeader->setAlignment( Qt::AlignCenter );
  layout->addWidget( gpuHeader );

  QGridLayout *gpuGrid = new QGridLayout();
  gpuGrid->setHorizontalSpacing( 24 );
  gpuGrid->setVerticalSpacing( 16 );
  gpuGrid->addWidget( makeGauge( "dGPU - Temp", "°C", m_gpuTempLabel ), 0, 0 );
  gpuGrid->addWidget( makeGauge( "dGPU - Frequency", "GHz", m_gpuFrequencyLabel ), 0, 1 );
  gpuGrid->addWidget( makeGauge( "dGPU - Fan", "%", m_gpuFanSpeedLabel ), 0, 2 );
  gpuGrid->addWidget( makeGauge( "dGPU - Power", "W", m_gpuPowerLabel ), 0, 3 );
  layout->addLayout( gpuGrid );

  layout->addStretch();
}

void DashboardTab::connectSignals()
{
  connect( m_systemMonitor, &SystemMonitor::cpuTempChanged,
           this, &DashboardTab::onCpuTempChanged );
  connect( m_systemMonitor, &SystemMonitor::cpuFrequencyChanged,
           this, &DashboardTab::onCpuFrequencyChanged );
  connect( m_systemMonitor, &SystemMonitor::cpuPowerChanged,
           this, &DashboardTab::onCpuPowerChanged );
  connect( m_systemMonitor, &SystemMonitor::gpuTempChanged,
           this, &DashboardTab::onGpuTempChanged );
  connect( m_systemMonitor, &SystemMonitor::gpuFrequencyChanged,
           this, &DashboardTab::onGpuFrequencyChanged );
  connect( m_systemMonitor, &SystemMonitor::gpuPowerChanged,
           this, &DashboardTab::onGpuPowerChanged );
  connect( m_systemMonitor, &SystemMonitor::fanSpeedChanged,
           this, &DashboardTab::onFanSpeedChanged );
  connect( m_systemMonitor, &SystemMonitor::gpuFanSpeedChanged,
           this, &DashboardTab::onGpuFanSpeedChanged );

  // Connect to profile manager for active profile changes
  connect( m_profileManager, &ProfileManager::activeProfileIndexChanged,
           this, [this]() {
             m_activeProfileLabel->setText( m_profileManager->activeProfile() );
           } );
}

// Dashboard slots
void DashboardTab::onCpuTempChanged()
{
  QString temp = m_systemMonitor->cpuTemp().replace( "°C", "" ).trimmed();
  bool ok = false;
  int tempValue = temp.toInt( &ok );

  if ( ok && tempValue > 0 )
  {
    m_cpuTempLabel->setText( temp );
  }
  else
  {
    m_cpuTempLabel->setText( "---" );
  }
}

void DashboardTab::onCpuFrequencyChanged()
{
  QString freq = m_systemMonitor->cpuFrequency();

  if ( freq.endsWith( " MHz" ) )
  {
    bool ok = false;
    double mhz = freq.left( freq.size() - 4 ).trimmed().toDouble( &ok );

    if ( ok )
    {
      if ( mhz > 0.0 )
      {
        double ghz = mhz / 1000.0;
        m_cpuFrequencyLabel->setText( QString::number( ghz, 'f', 1 ) );
        return;
      }
      m_cpuFrequencyLabel->setText( "--" );
      return;
    }
  }
  m_cpuFrequencyLabel->setText( freq.isEmpty() ? "--" : freq );
}

void DashboardTab::onCpuPowerChanged()
{
  QString power = m_systemMonitor->cpuPower();
  QString trimmed = power.replace( " W", "" ).trimmed();
  bool ok = false;
  double watts = trimmed.toDouble( &ok );

  if ( ok && watts > 0.0 )
  {
    m_cpuPowerLabel->setText( QString::number( watts, 'f', 1 ) );
    return;
  }
  m_cpuPowerLabel->setText( "--" );
}

void DashboardTab::onGpuTempChanged()
{
  QString temp = m_systemMonitor->gpuTemp().replace( "°C", "" ).trimmed();
  bool ok = false;
  int tempValue = temp.toInt( &ok );

  if ( ok && tempValue > 0 )
  {
    m_gpuTempLabel->setText( temp );
  }
  else
  {
    m_gpuTempLabel->setText( "---" );
  }
}

void DashboardTab::onGpuFrequencyChanged()
{
  QString freq = m_systemMonitor->gpuFrequency();

  if ( freq.endsWith( " MHz" ) )
  {
    bool ok = false;
    double mhz = freq.left( freq.size() - 4 ).trimmed().toDouble( &ok );

    if ( ok )
    {
      if ( mhz > 0.0 )
      {
        double ghz = mhz / 1000.0;
        m_gpuFrequencyLabel->setText( QString::number( ghz, 'f', 1 ) );
        return;
      }
      m_gpuFrequencyLabel->setText( "--" );
      return;
    }
  }
  m_gpuFrequencyLabel->setText( freq.isEmpty() ? "--" : freq );
}

void DashboardTab::onGpuPowerChanged()
{
  QString power = m_systemMonitor->gpuPower();
  QString trimmed = power.replace( " W", "" ).trimmed();
  bool ok = false;
  double watts = trimmed.toDouble( &ok );

  if ( ok && watts > 0.0 )
  {
    m_gpuPowerLabel->setText( QString::number( watts, 'f', 1 ) );
    return;
  }
  m_gpuPowerLabel->setText( "--" );
}

void DashboardTab::onFanSpeedChanged()
{
  m_fanSpeedLabel->setText( formatFanSpeed( m_systemMonitor->cpuFanSpeed() ) );
}

void DashboardTab::onGpuFanSpeedChanged()
{
  m_gpuFanSpeedLabel->setText( formatFanSpeed( m_systemMonitor->gpuFanSpeed() ) );
}

}