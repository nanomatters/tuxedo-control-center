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

#include "MainWindow.hpp"
#include "ProfileManager.hpp"
#include "SystemMonitor.hpp"


#include "FanCurveEditorWidget.hpp"
#include "../libucc-dbus/UccdClient.hpp"

#include "HardwareTab.hpp"

#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


// Helper widget for rotated y-axis label
class RotatedLabel : public QLabel
{
public:
  explicit RotatedLabel( const QString &text, QWidget *parent = nullptr )
    : QLabel( text, parent )
  {
  }

protected:
  void paintEvent( QPaintEvent *event ) override
  {
    ( void )event;
    QPainter p( this );
    p.setRenderHint( QPainter::Antialiasing );
    p.translate( width() / 2, height() / 2 );
    p.rotate( -90 );
    p.translate( -height() / 2, -width() / 2 );
    QRect r( 0, 0, height(), width() );
    p.setPen( QColor( "#bdbdbd" ) );
    QFont f = font();
    f.setPointSize( 11 );
    p.setFont( f );
    p.drawText( r, Qt::AlignCenter, "% Duty" );
  }

  QSize minimumSizeHint() const override
  {
    return QSize( 24, 80 );
  }

  QSize sizeHint() const override
  {
    return QSize( 24, 120 );
  }
};
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QTabWidget>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QScrollArea>
#include <QListWidget>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <cstdio>
#include <QPainter>

namespace ucc
{

MainWindow::MainWindow( QWidget *parent )
  : QMainWindow( parent )
  , m_profileManager( std::make_unique< ProfileManager >( this ) )
  , m_systemMonitor( std::make_unique< SystemMonitor >( this ) )
{
  m_UccdClient = std::make_unique< UccdClient >( this );

  setWindowTitle( "Uniwill Control Center" );
  setGeometry( 100, 100, 900, 700 );

  setupUI();

  // Connect signals after UI elements are created but before loading data
  connectSignals();

  // Initialize status bar
  statusBar()->showMessage( "Ready" );

  // Initialize hardware limits first before loading profiles
  std::vector< int > hardwareLimits = m_profileManager->getHardwarePowerLimits();
  qDebug() << "Constructor: Hardware limits initialized:";
  for ( size_t i = 0; i < hardwareLimits.size(); ++i )
  {
    qDebug() << "  Limit" << (int)i << "=" << hardwareLimits[i];
  }

  // Load initial data
  m_profileManager->refresh();
  
  // Ensure profile combo is populated after refresh
  onAllProfilesChanged();
  
  // Initialize current fan profile to first available fan profile (if any)
  m_currentFanProfile = ( m_fanProfileCombo && m_fanProfileCombo->count() > 0 ) ? m_fanProfileCombo->currentText() : QString();
  
  // Update fan tab visibility now that profiles are loaded
  updateFanTabVisibility();

  // Start monitoring since dashboard is the first tab
  m_systemMonitor->setMonitoringActive( true );
}

MainWindow::~MainWindow()
{
  // Destructor
}

void MainWindow::setupUI()
{
  // Create tab widget
  m_tabs = new QTabWidget( this );
  setCentralWidget( m_tabs );

  // Connect tab changes to control monitoring
  connect( m_tabs, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged );

  // Create HardwareTab first to get water cooler controller
  m_hardwareTab = new HardwareTab( m_systemMonitor.get(), this );
  m_tabs->addTab( m_hardwareTab, "Hardware" );

  // Now create DashboardTab with water cooler controller
  m_dashboardTab = new DashboardTab( m_systemMonitor.get(), m_profileManager.get(), m_hardwareTab->getWaterCoolerController(), this );
  m_tabs->addTab( m_dashboardTab, "Dashboard" );
  setupProfilesPage();

  // Place the Fan Control tab directly after Profiles and rename it
  setupFanControlTab();
  updateFanTabVisibility(); // Set initial visibility based on current profile
  setupKeyboardBacklightPage();
}

void MainWindow::setupFanControlTab()
{
  m_fanTab = new QWidget();
  m_fanTab->setStyleSheet( "background-color: #242424; color: #e6e6e6;" );
  QVBoxLayout *mainLayout = new QVBoxLayout( m_fanTab );
  mainLayout->setContentsMargins( 0, 0, 0, 0 );
  mainLayout->setSpacing( 0 );

  // Create scroll area for the fan control content
  QScrollArea *scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable( true );
  
  QWidget *scrollWidget = new QWidget();
  QVBoxLayout *scrollLayout = new QVBoxLayout( scrollWidget );
  scrollLayout->setContentsMargins( 20, 20, 20, 20 );
  scrollLayout->setSpacing( 15 );

  // Top controls: profile selection
  QHBoxLayout *selectLayout = new QHBoxLayout();
  QLabel *profileLabel = new QLabel( "Active Profile:" );
  profileLabel->setStyleSheet( "font-weight: bold;" );
  m_fanProfileCombo = new QComboBox();

  if ( auto names = m_UccdClient->getFanProfileNames() )
  {
    m_builtinFanProfiles.clear();
    for ( const auto &name : *names ) {
      QString qn = QString::fromStdString( name );
      m_fanProfileCombo->addItem( qn );
      m_builtinFanProfiles.append( qn );
    }
  }

  // Append persisted custom fan profiles loaded from settings
  for ( const auto &name : m_profileManager->customFanProfiles() )
  {
    if ( m_fanProfileCombo->findText( name ) == -1 )
      m_fanProfileCombo->addItem( name );
  }
  
  m_applyFanProfilesButton = new QPushButton("Apply");
  m_applyFanProfilesButton->setMaximumWidth(80);
  m_applyFanProfilesButton->setEnabled( false );
  
  m_saveFanProfilesButton = new QPushButton("Save");
  m_saveFanProfilesButton->setMaximumWidth(80);
  m_saveFanProfilesButton->setEnabled( false );
  
  m_addFanProfileButton = new QPushButton("Add");
  m_addFanProfileButton->setMaximumWidth(60);
  
  m_copyFanProfileButton = new QPushButton("Copy");
  m_copyFanProfileButton->setMaximumWidth(60);
  m_copyFanProfileButton->setEnabled( false );
  
  m_removeFanProfileButton = new QPushButton("Remove");
  m_removeFanProfileButton->setMaximumWidth(70);
  
  selectLayout->addWidget( profileLabel );
  selectLayout->addWidget( m_fanProfileCombo, 1 );
  selectLayout->addWidget(m_applyFanProfilesButton);
  selectLayout->addWidget(m_saveFanProfilesButton);
  selectLayout->addWidget(m_addFanProfileButton);
  selectLayout->addWidget(m_copyFanProfileButton);
  selectLayout->addWidget(m_removeFanProfileButton);
  mainLayout->addLayout( selectLayout );

  // Add a separator line
  QFrame *separator = new QFrame();
  separator->setFrameShape( QFrame::HLine );
  separator->setStyleSheet( "color: #cccccc;" );
  mainLayout->addWidget( separator );

  // CPU and GPU fan curve editors
  // CPU fan curve editor
  QVBoxLayout *cpuLayout = new QVBoxLayout();
  cpuLayout->setSpacing( 0 );
  QLabel *cpuLabel = new QLabel( "CPU Fan Curve" );
  cpuLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
  cpuLayout->addWidget( cpuLabel );
  m_cpuFanCurveEditor = new FanCurveEditorWidget();
  m_cpuFanCurveEditor->setStyleSheet( "background-color: #1a1a1a; border: 1px solid #555;" );
  cpuLayout->addWidget( m_cpuFanCurveEditor );
  scrollLayout->addLayout( cpuLayout );
  
  // GPU fan curve editor
  QVBoxLayout *gpuLayout = new QVBoxLayout();
  gpuLayout->setSpacing( 0 );
  QLabel *gpuLabel = new QLabel( "GPU Fan Curve" );
  gpuLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
  gpuLayout->addWidget( gpuLabel );
  m_gpuFanCurveEditor = new FanCurveEditorWidget();
  m_gpuFanCurveEditor->setStyleSheet( "background-color: #1a1a1a; border: 1px solid #555;" );
  gpuLayout->addWidget( m_gpuFanCurveEditor );
  scrollLayout->addLayout( gpuLayout );

  // Connect signals
  connect( m_fanProfileCombo, &QComboBox::currentTextChanged,
           this, &MainWindow::onFanProfileChanged );
  connect( m_cpuFanCurveEditor, &FanCurveEditorWidget::pointsChanged,
           this, &MainWindow::onCpuFanPointsChanged );
  connect( m_gpuFanCurveEditor, &FanCurveEditorWidget::pointsChanged,
           this, &MainWindow::onGpuFanPointsChanged );
  connect( m_addFanProfileButton, &QPushButton::clicked,
           this, &MainWindow::onAddFanProfileClicked );
  connect( m_copyFanProfileButton, &QPushButton::clicked,
           this, &MainWindow::onCopyFanProfileClicked );
  connect( m_removeFanProfileButton, &QPushButton::clicked,
           this, &MainWindow::onRemoveFanProfileClicked );

  // Fan profile action buttons
  connect( m_applyFanProfilesButton, &QPushButton::clicked,
           this, &MainWindow::onApplyFanProfilesClicked );
  connect( m_saveFanProfilesButton, &QPushButton::clicked,
           this, &MainWindow::onSaveFanProfilesClicked );
  // Note: revert button may be added later; connect when created


  scrollArea->setWidget( scrollWidget );
  mainLayout->addWidget( scrollArea );

  // Load initial fan profile selection if available
  if ( m_fanProfileCombo->count() > 0 )
    onFanProfileChanged( m_fanProfileCombo->currentText() );

}

void MainWindow::updateFanTabVisibility()
{
  qDebug() << "=== updateFanTabVisibility called (modified: always show fan tab) ===";
  bool currentlyVisible = (m_tabs->indexOf(m_fanTab) != -1);

  qDebug() << "updateFanTabVisibility: currentlyVisible=" << currentlyVisible;

  // Always ensure the fan tab is available; do not hide it for built-in profiles
  if ( !currentlyVisible )
  {
    qDebug() << "Adding fan tab (always visible now)";
    m_tabs->addTab(m_fanTab, "Profile Fan Control");
  }
}

void MainWindow::setupProfilesPage()
{
  QWidget *profilesWidget = new QWidget();
  QVBoxLayout *mainLayout = new QVBoxLayout( profilesWidget );
  mainLayout->setContentsMargins( 0, 0, 0, 0 );
  mainLayout->setSpacing( 0 );

  // Create scroll area for the profile content
  QScrollArea *scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable( true );
  
  QWidget *scrollWidget = new QWidget();
  QVBoxLayout *scrollLayout = new QVBoxLayout( scrollWidget );
  scrollLayout->setContentsMargins( 20, 20, 20, 20 );
  scrollLayout->setSpacing( 15 );

  // Profile Selection ComboBox (in top layout)
  QHBoxLayout *selectLayout = new QHBoxLayout();
  QLabel *selectLabel = new QLabel( "Active Profile:" );
  selectLabel->setStyleSheet( "font-weight: bold;" );

  m_profileCombo = new QComboBox();
  // Don't populate here - will be done by onAllProfilesChanged signal
  m_profileCombo->setCurrentIndex( m_profileManager->activeProfileIndex() );
  m_selectedProfileIndex = m_profileManager->activeProfileIndex();
  m_applyButton = new QPushButton( "Activate" );
  m_applyButton->setMaximumWidth( 80 );
  
  m_saveButton = new QPushButton( "Save" );
  m_saveButton->setMaximumWidth( 80 );
  m_saveButton->setEnabled( false );
  
  m_copyProfileButton = new QPushButton( "Copy" );
  m_copyProfileButton->setMaximumWidth( 60 );
  
  m_removeProfileButton = new QPushButton( "Remove" );
  m_removeProfileButton->setMaximumWidth( 70 );
  
  selectLayout->addWidget( selectLabel );
  selectLayout->addWidget( m_profileCombo, 1 );
  selectLayout->addWidget( m_applyButton );
  selectLayout->addWidget( m_saveButton );
  selectLayout->addWidget( m_copyProfileButton );
  selectLayout->addWidget( m_removeProfileButton );
  mainLayout->addLayout( selectLayout );

  // Add a separator line
  QFrame *separator = new QFrame();
  separator->setFrameShape( QFrame::HLine );
  separator->setStyleSheet( "color: #cccccc;" );
  mainLayout->addWidget( separator );

  // Now use grid layout for the details
  scrollLayout->setContentsMargins( 15, 10, 15, 10 );
  QGridLayout *detailsLayout = new QGridLayout();
  detailsLayout->setSpacing( 12 );
  detailsLayout->setColumnStretch( 0, 0 );  // Labels column - minimal width
  detailsLayout->setColumnStretch( 1, 1 );  // Controls column - expand

  int row = 0;

  // === NAME ===
  QLabel *nameLabel = new QLabel( "Name" );
  nameLabel->setStyleSheet( "font-weight: bold;" );
  m_nameEdit = new QLineEdit();
  m_nameEdit->setPlaceholderText( "Profile name" );
  m_nameEdit->setReadOnly( true );
  detailsLayout->addWidget( nameLabel, row, 0, Qt::AlignTop );
  detailsLayout->addWidget( m_nameEdit, row, 1 );
  row++;

  // === DESCRIPTION ===
  QLabel *descLabel = new QLabel( "Description" );
  descLabel->setStyleSheet( "font-weight: bold;" );
  m_descriptionEdit = new QTextEdit();
  m_descriptionEdit->setPlainText( "Edit profile to change behaviour" );
  m_descriptionEdit->setMaximumHeight( 60 );
  detailsLayout->addWidget( descLabel, row, 0, Qt::AlignTop );
  detailsLayout->addWidget( m_descriptionEdit, row, 1 );
  row++;

  // === ACTIVATE PROFILE AUTOMATICALLY ON ===
  QLabel *autoActivateLabel = new QLabel( "Activate profile automatically on" );
  autoActivateLabel->setStyleSheet( "font-weight: bold;" );
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  m_mainsButton = new QPushButton( "Mains" );
  m_batteryButton = new QPushButton( "Battery" );
  m_mainsButton->setCheckable( true );
  m_batteryButton->setCheckable( true );
  m_mainsButton->setChecked( true );
  m_mainsButton->setMaximumWidth( 100 );
  m_batteryButton->setMaximumWidth( 100 );
  buttonLayout->addWidget( m_mainsButton );
  buttonLayout->addWidget( m_batteryButton );
  buttonLayout->addStretch();
  detailsLayout->addWidget( autoActivateLabel, row, 0, Qt::AlignTop );
  detailsLayout->addLayout( buttonLayout, row, 1 );
  row++;

  // Add spacer/separator
  detailsLayout->addItem( new QSpacerItem( 0, 15 ), row, 0, 1, 2 );
  row++;

  // === DISPLAY SECTION ===
  QLabel *displayHeader = new QLabel( "Display and Keyboard" );
  displayHeader->setStyleSheet( "font-weight: bold; font-size: 14px;" );
  detailsLayout->addWidget( displayHeader, row, 0, 1, 2 );
  row++;

  QLabel *keyboardProfileLabel = new QLabel( "Keyboard profile" );
  m_profileKeyboardProfileCombo = new QComboBox();

  for ( const auto &name : m_profileManager->customKeyboardProfiles() )
    m_profileKeyboardProfileCombo->addItem( name );

  detailsLayout->addWidget( keyboardProfileLabel, row, 0 );
  detailsLayout->addWidget( m_profileKeyboardProfileCombo, row, 1 );
  row++;

  QLabel *setBrightnessLabel = new QLabel( "Set brightness on profile activation" );
  m_setBrightnessCheckBox = new QCheckBox();
  m_setBrightnessCheckBox->setChecked( false );
  detailsLayout->addWidget( setBrightnessLabel, row, 0 );
  detailsLayout->addWidget( m_setBrightnessCheckBox, row, 1, Qt::AlignLeft );
  row++;

  QLabel *backlightLabel = new QLabel( "Backlight brightness" );
  backlightLabel->setStyleSheet( "font-style: italic; color: #999999;" );
  QHBoxLayout *backlightLayout = new QHBoxLayout();
  m_brightnessSlider = new QSlider( Qt::Horizontal );
  m_brightnessSlider->setMinimum( 0 );
  m_brightnessSlider->setMaximum( 100 );
  m_brightnessSlider->setValue( 100 );
  m_brightnessValueLabel = new QLabel( "100%" );
  m_brightnessValueLabel->setMinimumWidth( 40 );
  backlightLayout->addWidget( m_brightnessSlider, 1 );
  backlightLayout->addWidget( m_brightnessValueLabel );
  detailsLayout->addWidget( backlightLabel, row, 0 );
  detailsLayout->addLayout( backlightLayout, row, 1 );
  row++;

  // Add spacer
  detailsLayout->addItem( new QSpacerItem( 0, 10 ), row, 0, 1, 2 );
  row++;

  // === FAN CONTROL SECTION ===
  QLabel *fanHeader = new QLabel( "Fan control" );
  fanHeader->setStyleSheet( "font-weight: bold; font-size: 14px;" );
  detailsLayout->addWidget( fanHeader, row, 0, 1, 2 );
  row++;

  QLabel *fanProfileLabel = new QLabel( "Fan profile" );
  m_profileFanProfileCombo = new QComboBox();
  if ( auto names = m_UccdClient->getFanProfileNames() )
  {
    for ( const auto &name : *names )
      m_profileFanProfileCombo->addItem( QString::fromStdString( name ) );
  }
  // Append persisted custom fan profiles loaded from settings
  for ( const auto &name : m_profileManager->customFanProfiles() )
  {
    if ( m_profileFanProfileCombo->findText( name ) == -1 )
      m_profileFanProfileCombo->addItem( name );
  }
  detailsLayout->addWidget( fanProfileLabel, row, 0 );
  detailsLayout->addWidget( m_profileFanProfileCombo, row, 1 );
  row++;

  QLabel *minFanLabel = new QLabel( "Minimum fan speed" );
  minFanLabel->setStyleSheet( "font-style: italic; color: #999999;" );
  QHBoxLayout *minFanLayout = new QHBoxLayout();
  m_minFanSpeedSlider = new QSlider( Qt::Horizontal );
  m_minFanSpeedSlider->setMinimum( 0 );
  m_minFanSpeedSlider->setMaximum( 100 );
  m_minFanSpeedSlider->setValue( 0 );
  m_minFanSpeedValue = new QLabel( "0%" );
  m_minFanSpeedValue->setMinimumWidth( 40 );
  minFanLayout->addWidget( m_minFanSpeedSlider, 1 );
  minFanLayout->addWidget( m_minFanSpeedValue );
  detailsLayout->addWidget( minFanLabel, row, 0 );
  detailsLayout->addLayout( minFanLayout, row, 1 );
  row++;

  QLabel *maxFanLabel = new QLabel( "Maximum fan speed" );
  maxFanLabel->setStyleSheet( "font-style: italic; color: #999999;" );
  QHBoxLayout *maxFanLayout = new QHBoxLayout();
  m_maxFanSpeedSlider = new QSlider( Qt::Horizontal );
  m_maxFanSpeedSlider->setMinimum( 0 );
  m_maxFanSpeedSlider->setMaximum( 100 );
  m_maxFanSpeedSlider->setValue( 70 );
  m_maxFanSpeedValue = new QLabel( "70%" );
  m_maxFanSpeedValue->setMinimumWidth( 40 );
  maxFanLayout->addWidget( m_maxFanSpeedSlider, 1 );
  maxFanLayout->addWidget( m_maxFanSpeedValue );
  detailsLayout->addWidget( maxFanLabel, row, 0 );
  detailsLayout->addLayout( maxFanLayout, row, 1 );
  row++;

  QLabel *offsetFanLabel = new QLabel( "Offset fan speed" );
  offsetFanLabel->setStyleSheet( "font-style: italic; color: #999999;" );
  QHBoxLayout *offsetFanLayout = new QHBoxLayout();
  m_offsetFanSpeedSlider = new QSlider( Qt::Horizontal );
  m_offsetFanSpeedSlider->setMinimum( -30 );
  m_offsetFanSpeedSlider->setMaximum( 30 );
  m_offsetFanSpeedSlider->setValue( -2 );
  m_offsetFanSpeedValue = new QLabel( "-2%" );
  m_offsetFanSpeedValue->setMinimumWidth( 40 );
  offsetFanLayout->addWidget( m_offsetFanSpeedSlider, 1 );
  offsetFanLayout->addWidget( m_offsetFanSpeedValue );
  detailsLayout->addWidget( offsetFanLabel, row, 0 );
  detailsLayout->addLayout( offsetFanLayout, row, 1 );
  row++;

  QLabel *sameSpeedLabel = new QLabel( "Same fan speed for all fans" );
  detailsLayout->addWidget( sameSpeedLabel, row, 0 );
  // Reuse the shared checkbox created in the dashboard (create if not present)
  if ( !m_sameFanSpeedCheckBox ) {
    m_sameFanSpeedCheckBox = new QCheckBox();
    m_sameFanSpeedCheckBox->setChecked( true );
  }
  detailsLayout->addWidget( m_sameFanSpeedCheckBox, row, 1, Qt::AlignLeft );
  row++;

  // Add spacer
  detailsLayout->addItem( new QSpacerItem( 0, 10 ), row, 0, 1, 2 );
  row++;

  // === SYSTEM PERFORMANCE SECTION ===
  QLabel *sysHeader = new QLabel( "System performance" );
  sysHeader->setStyleSheet( "font-weight: bold; font-size: 14px;" );
  detailsLayout->addWidget( sysHeader, row, 0, 1, 2 );
  row++;

  QLabel *odmPowerHeader = new QLabel( "CPU power limit control" );
  odmPowerHeader->setStyleSheet( "font-style: italic; color: #666666;" );
  detailsLayout->addWidget( odmPowerHeader, row, 0, 1, 2 );
  row++;

  // TDP Limit 1
  QLabel *tdp1Label = new QLabel( "Sustained TDP" );  // Sustained Power Limit
  QHBoxLayout *tdp1Layout = new QHBoxLayout();
  m_odmPowerLimit1Slider = new QSlider( Qt::Horizontal );
  m_odmPowerLimit1Slider->setMinimum( 0 );
  m_odmPowerLimit1Slider->setMaximum( 250 );  // Will be updated from hardware limits in loadProfileDetails
  m_odmPowerLimit1Slider->setValue( 0 );
  m_odmPowerLimit1Value = new QLabel( "0 W" );
  m_odmPowerLimit1Value->setMinimumWidth( 50 );
  tdp1Layout->addWidget( m_odmPowerLimit1Slider, 1 );
  tdp1Layout->addWidget( m_odmPowerLimit1Value );
  detailsLayout->addWidget( tdp1Label, row, 0 );
  detailsLayout->addLayout( tdp1Layout, row, 1 );
  row++;

  // TDP Limit 2
  QLabel *tdp2Label = new QLabel( "Boost TDP" );
  QHBoxLayout *tdp2Layout = new QHBoxLayout();
  m_odmPowerLimit2Slider = new QSlider( Qt::Horizontal );
  m_odmPowerLimit2Slider->setMinimum( 0 );
  m_odmPowerLimit2Slider->setMaximum( 250 );  // Will be updated from hardware limits in loadProfileDetails
  m_odmPowerLimit2Slider->setValue( 0 );
  m_odmPowerLimit2Value = new QLabel( "0 W" );
  m_odmPowerLimit2Value->setMinimumWidth( 50 );
  tdp2Layout->addWidget( m_odmPowerLimit2Slider, 1 );
  tdp2Layout->addWidget( m_odmPowerLimit2Value );
  detailsLayout->addWidget( tdp2Label, row, 0 );
  detailsLayout->addLayout( tdp2Layout, row, 1 );
  row++;

  // TDP Limit 3
  QLabel *tdp3Label = new QLabel( "Peak TDP" );
  QHBoxLayout *tdp3Layout = new QHBoxLayout();
  m_odmPowerLimit3Slider = new QSlider( Qt::Horizontal );
  m_odmPowerLimit3Slider->setMinimum( 0 );
  m_odmPowerLimit3Slider->setMaximum( 250 );  // Will be updated from hardware limits in loadProfileDetails
  m_odmPowerLimit3Slider->setValue( 0 );
  m_odmPowerLimit3Value = new QLabel( "0 W" );
  m_odmPowerLimit3Value->setMinimumWidth( 50 );
  tdp3Layout->addWidget( m_odmPowerLimit3Slider, 1 );
  tdp3Layout->addWidget( m_odmPowerLimit3Value );
  detailsLayout->addWidget( tdp3Label, row, 0 );
  detailsLayout->addLayout( tdp3Layout, row, 1 );
  row++;

  // Add spacer
  detailsLayout->addItem( new QSpacerItem( 0, 5 ), row, 0, 1, 2 );
  row++;

  QLabel *cpuFreqHeader = new QLabel( "CPU frequency control" );
  cpuFreqHeader->setStyleSheet( "font-style: italic; color: #999999;" );
  detailsLayout->addWidget( cpuFreqHeader, row, 0, 1, 2 );
  row++;

  QLabel *coresLabel = new QLabel( "Number of logical cores" );
  QHBoxLayout *coresLayout = new QHBoxLayout();
  m_cpuCoresSlider = new QSlider( Qt::Horizontal );
  m_cpuCoresSlider->setMinimum( 1 );
  m_cpuCoresSlider->setMaximum( 32 );
  m_cpuCoresSlider->setValue( 32 );
  m_cpuCoresValue = new QLabel( "32" );
  m_cpuCoresValue->setMinimumWidth( 35 );
  coresLayout->addWidget( m_cpuCoresSlider, 1 );
  coresLayout->addWidget( m_cpuCoresValue );
  detailsLayout->addWidget( coresLabel, row, 0 );
  detailsLayout->addLayout( coresLayout, row, 1 );
  row++;

  QLabel *maxPerfLabel = new QLabel( "CPU Governor" );
  m_governorCombo = new QComboBox();
  m_governorCombo->addItem( "powersave", "powersave" );
  m_governorCombo->addItem( "performance", "performance" );
  detailsLayout->addWidget( maxPerfLabel, row, 0 );
  detailsLayout->addWidget( m_governorCombo, row, 1, Qt::AlignLeft );
  row++;

  QLabel *minFreqLabel = new QLabel( "Minimum frequency" );
  minFreqLabel->setStyleSheet( "font-style: italic; color: #999999;" );
  QHBoxLayout *minFreqLayout = new QHBoxLayout();
  m_minFrequencySlider = new QSlider( Qt::Horizontal );
  m_minFrequencySlider->setMinimum( 500 );
  m_minFrequencySlider->setMaximum( 5000 );
  m_minFrequencySlider->setValue( 500 );
  m_minFrequencyValue = new QLabel( "0.5 GHz" );
  m_minFrequencyValue->setMinimumWidth( 60 );
  minFreqLayout->addWidget( m_minFrequencySlider, 1 );
  minFreqLayout->addWidget( m_minFrequencyValue );
  detailsLayout->addWidget( minFreqLabel, row, 0 );
  detailsLayout->addLayout( minFreqLayout, row, 1 );
  row++;

  QLabel *maxFreqLabel = new QLabel( "Maximum frequency" );
  maxFreqLabel->setStyleSheet( "font-style: italic; color: #999999;" );
  QHBoxLayout *maxFreqLayout = new QHBoxLayout();
  m_maxFrequencySlider = new QSlider( Qt::Horizontal );
  m_maxFrequencySlider->setMinimum( 500 );
  m_maxFrequencySlider->setMaximum( 5000 );
  m_maxFrequencySlider->setValue( 5500 );
  m_maxFrequencySlider->setSingleStep( 100 );
  m_maxFrequencyValue = new QLabel( "5.5 GHz" );
  m_maxFrequencyValue->setMinimumWidth( 60 );
  maxFreqLayout->addWidget( m_maxFrequencySlider, 1 );
  maxFreqLayout->addWidget( m_maxFrequencyValue );
  detailsLayout->addWidget( maxFreqLabel, row, 0 );
  detailsLayout->addLayout( maxFreqLayout, row, 1 );
  row++;

  // Add spacer
  detailsLayout->addItem( new QSpacerItem( 0, 10 ), row, 0, 1, 2 );
  row++;

  // === NVIDIA POWER CONTROL ===
  QLabel *nvidiaHeader = new QLabel( "NVIDIA power control" );
  nvidiaHeader->setStyleSheet( "font-weight: bold; font-size: 14px;" );
  detailsLayout->addWidget( nvidiaHeader, row, 0, 1, 2 );
  row++;

  QLabel *gpuPowerLabel = new QLabel( "Configurable graphics power (TGP)" );
  gpuPowerLabel->setStyleSheet( "font-style: italic; color: #999999;" );
  QHBoxLayout *gpuLayout = new QHBoxLayout();
  m_gpuPowerSlider = new QSlider( Qt::Horizontal );
  m_gpuPowerSlider->setMinimum( 40 );
  m_gpuPowerSlider->setMaximum( 175 );
  m_gpuPowerSlider->setValue( 175 );
  m_gpuPowerValue = new QLabel( "175 W" );
  m_gpuPowerValue->setMinimumWidth( 50 );
  gpuLayout->addWidget( m_gpuPowerSlider, 1 );
  gpuLayout->addWidget( m_gpuPowerValue );
  detailsLayout->addWidget( gpuPowerLabel, row, 0 );
  detailsLayout->addLayout( gpuLayout, row, 1 );
  row++;

  detailsLayout->addItem( new QSpacerItem( 0, 20, QSizePolicy::Minimum, QSizePolicy::Expanding ), row, 0, 1, 2 );

  scrollLayout->addLayout( detailsLayout );

  scrollArea->setWidget( scrollWidget );
  mainLayout->addWidget( scrollArea );

  m_tabs->addTab( profilesWidget, "Profiles" );
}

void MainWindow::connectSignals()
{
  // Profile page connections

  connect( m_profileManager.get(), &ProfileManager::allProfilesChanged,
           this, &MainWindow::onAllProfilesChanged );

  connect( m_profileManager.get(), &ProfileManager::activeProfileIndexChanged,
           this, &MainWindow::onActiveProfileIndexChanged );

  connect( m_profileManager.get(), &ProfileManager::activeProfileChanged,
           this, [this]() {
    qDebug() << "activeProfileChanged signal received, updating UI";
    loadProfileDetails( m_profileManager->activeProfile() );
    updateFanTabVisibility();
  } );

  connect( m_profileManager.get(), &ProfileManager::customKeyboardProfilesChanged,
           this, &MainWindow::onCustomKeyboardProfilesChanged );

  connect( m_profileCombo, QOverload< int >::of( &QComboBox::currentIndexChanged ),
           this, &MainWindow::onProfileIndexChanged );

  // Display controls

  connect( m_brightnessSlider, &QSlider::valueChanged,
           this, &MainWindow::onBrightnessSliderChanged );

  // Fan controls

  connect( m_minFanSpeedSlider, &QSlider::valueChanged,
           this, &MainWindow::onMinFanSpeedChanged );

  connect( m_maxFanSpeedSlider, &QSlider::valueChanged,
           this, &MainWindow::onMaxFanSpeedChanged );

  connect( m_offsetFanSpeedSlider, &QSlider::valueChanged,
           this, &MainWindow::onOffsetFanSpeedChanged );

  // ODM Power Limit controls

  connect( m_odmPowerLimit1Slider, &QSlider::valueChanged,
           this, &MainWindow::onODMPowerLimit1Changed );

  connect( m_odmPowerLimit2Slider, &QSlider::valueChanged,
           this, &MainWindow::onODMPowerLimit2Changed );

  connect( m_odmPowerLimit3Slider, &QSlider::valueChanged,
           this, &MainWindow::onODMPowerLimit3Changed );

  // CPU frequency controls

  connect( m_cpuCoresSlider, &QSlider::valueChanged,
           this, &MainWindow::onCpuCoresChanged );

  connect( m_maxFrequencySlider, &QSlider::valueChanged,
           this, &MainWindow::onMaxFrequencyChanged );

  connect( m_minFrequencySlider, &QSlider::valueChanged,
           this, [this]( int value ) {
    double freqGHz = value / 1000.0;
    m_minFrequencyValue->setText( QString::number( freqGHz, 'f', 1 ) + " GHz" );
  } );

  // GPU power control

  connect( m_gpuPowerSlider, &QSlider::valueChanged,
           this, &MainWindow::onGpuPowerChanged );

  // Apply and Save buttons

  connect( m_applyButton, &QPushButton::clicked,
           this, &MainWindow::onApplyClicked );

  connect( m_saveButton, &QPushButton::clicked,
           this, &MainWindow::onSaveClicked );

  connect( m_copyProfileButton, &QPushButton::clicked,
           this, &MainWindow::onCopyProfileClicked );

  connect( m_removeProfileButton, &QPushButton::clicked,
           this, &MainWindow::onRemoveProfileClicked );

  // Connect all profile controls to mark changes

  connect( m_descriptionEdit, &QTextEdit::textChanged,
           this, &MainWindow::markChanged );

  if ( m_nameEdit ) {
    connect( m_nameEdit, &QLineEdit::textChanged,
             this, &MainWindow::markChanged );
  }

  connect( m_setBrightnessCheckBox, &QCheckBox::toggled,
           this, &MainWindow::markChanged );

  connect( m_brightnessSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_profileFanProfileCombo, QOverload< int >::of( &QComboBox::currentIndexChanged ),
           this, [this](int index) {
             markChanged();
             // Update fan profile tab to match profile tab selection
             m_fanProfileCombo->blockSignals(true);
             m_fanProfileCombo->setCurrentIndex(index);
             m_fanProfileCombo->blockSignals(false);
             // Load the fan curves for the new profile
             onFanProfileChanged(m_profileFanProfileCombo->currentText());
           } );

  connect( m_minFanSpeedSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_maxFanSpeedSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_offsetFanSpeedSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_cpuCoresSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_governorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
           this, &MainWindow::markChanged );

  connect( m_minFrequencySlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_maxFrequencySlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_odmPowerLimit1Slider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_odmPowerLimit2Slider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_odmPowerLimit3Slider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_gpuPowerSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_profileKeyboardProfileCombo, QOverload< int >::of( &QComboBox::currentIndexChanged ),
           this, &MainWindow::markChanged );

  // Mains/Battery activation buttons
  connect( m_mainsButton, &QPushButton::toggled,
           this, &MainWindow::markChanged );

  connect( m_batteryButton, &QPushButton::toggled,
           this, &MainWindow::markChanged );

  // Error handling

  connect( m_profileManager.get(), QOverload< const QString & >::of( &ProfileManager::error ),
           this, [this]( const QString &msg ) {
    statusBar()->showMessage( "Error: " + msg );
    m_saveInProgress = false;
    updateButtonStates();
  } );

  // React to DBus client connection status so we can (re)load built-in fan profiles
  connect( m_UccdClient.get(), &UccdClient::connectionStatusChanged,
           this, &MainWindow::onUccdConnectionChanged );

  connectKeyboardBacklightPageWidgets();

  // Initial load of fan profiles (may be empty if service not yet available)
  reloadFanProfiles();

  // Populate governor combo
  populateGovernorCombo();
}

void MainWindow::populateGovernorCombo()
{
  if ( !m_governorCombo )
    return;

  m_governorCombo->clear();

  if ( auto governors = m_UccdClient->getAvailableCpuGovernors(); governors && !governors->empty() )
  {
    for ( const auto &gov : *governors )
      m_governorCombo->addItem( QString::fromStdString( gov ), QString::fromStdString( gov ) );
  }
}

void MainWindow::onTabChanged( int index )
{
  // Enable monitoring only when dashboard tab (index 0) is visible
  bool isDashboard = ( index == 0 );
  qDebug() << "Tab changed to" << index << "- Monitoring active:" << isDashboard;
  m_systemMonitor->setMonitoringActive( isDashboard );

  // Load current keyboard backlight states when keyboard tab (index 4) is activated
  if ( index == 4 )
  {
    if ( m_keyboardVisualizer )
    {
      if ( auto states = m_UccdClient->getKeyboardBacklightStates() )
      {
        m_keyboardVisualizer->loadCurrentStates( *states );
        qDebug() << "Loaded current keyboard backlight states";
      }
    }
    
    // Reload keyboard profiles
    reloadKeyboardProfiles();

    // Auto-load the keyboard profile from the active profile's settings (by name reference)
    QString activeProfile = m_profileManager->activeProfile();
    if ( !activeProfile.isEmpty() )
    {
      QString profileJson = m_profileManager->getProfileDetails( m_profileManager->getProfileIdByName( activeProfile ) );
      if ( !profileJson.isEmpty() )
      {
        QJsonDocument doc = QJsonDocument::fromJson( profileJson.toUtf8() );
        if ( doc.isObject() )
        {
          QJsonObject obj = doc.object();
          if ( obj.contains( "keyboard" ) && obj["keyboard"].isObject() )
          {
            QJsonObject keyboardObj = obj["keyboard"].toObject();
            QString keyboardProfile = keyboardObj["profile"].toString();
            if ( !keyboardProfile.isEmpty() && m_keyboardProfileCombo->findText( keyboardProfile ) != -1 )
            {
              m_keyboardProfileCombo->setCurrentText( keyboardProfile );
              // This triggers onKeyboardProfileChanged to load the profile data from UCC settings
            }
          }
        }
      }
    }
  }
}

// Profile page slots
void MainWindow::onCustomKeyboardProfilesChanged()
{
  // Repopulate keyboard profile combos
  m_profileKeyboardProfileCombo->clear();
  for ( const auto &name : m_profileManager->customKeyboardProfiles() )
    m_profileKeyboardProfileCombo->addItem( name );

  reloadKeyboardProfiles();
}

void MainWindow::onProfileIndexChanged( int index )
{
  if ( index >= 0 )
  {
    QString profileName = m_profileCombo->currentText();
    qDebug() << "Profile selected:" << profileName << "at index" << index;
    m_selectedProfileIndex = index;
    loadProfileDetails( profileName );
    m_removeProfileButton->setEnabled( m_profileManager->customProfiles().contains( profileName ) );
    m_copyProfileButton->setEnabled( true );
    m_saveButton->setEnabled( true );
    statusBar()->showMessage( "Profile selected: " + profileName + " (click Apply to activate)" );
  }
}

void MainWindow::onAllProfilesChanged()
{
  FILE *log = fopen( "/tmp/ucc-debug.log", "a" );
  fprintf( log, "\n\n!!! onAllProfilesChanged() CALLED !!!\n\n" );
  fflush( log );
  fclose( log );
  
  FILE *log2 = fopen( "/tmp/ucc-debug.log", "a" );
  fprintf( log2, "onAllProfilesChanged called\n" );
  
  // First, ensure hardware limits are loaded
  std::vector< int > hardwareLimits = m_profileManager->getHardwarePowerLimits();
  fprintf( log2, "Hardware limits in onAllProfilesChanged: %zu values\n", hardwareLimits.size() );
  for ( size_t i = 0; i < hardwareLimits.size(); ++i )
  {
    fprintf( log2, "  Limit %zu = %d\n", i, hardwareLimits[i] );
  }
  fflush( log2 );
  
  m_profileCombo->clear();
  m_profileCombo->addItems( m_profileManager->allProfiles() );
  m_profileCombo->setCurrentIndex( m_profileManager->activeProfileIndex() );
  m_selectedProfileIndex = m_profileManager->activeProfileIndex();

  // Load the active profile details
  QString activeProfile = m_profileManager->activeProfile();
  fprintf( log2, "Active profile: %s (index: %d)\n", activeProfile.toStdString().c_str(), 
           m_profileManager->activeProfileIndex() );
  fprintf( log2, "All profiles: " );
  for ( const auto &p : m_profileManager->allProfiles() )
  {
    fprintf( log2, "%s, ", p.toStdString().c_str() );
  }
  fprintf( log2, "\n" );
  fflush( log2 );
  
  if ( !activeProfile.isEmpty() )
  {
    fprintf( log2, "Calling loadProfileDetails for: %s\n", activeProfile.toStdString().c_str() );
    fflush( log2 );
    loadProfileDetails( activeProfile );
  }
  else
  {
    fprintf( log2, "ERROR: Active profile is empty!\n" );
    fflush( log2 );
  }

  // Custom profiles may have changed; reload fan profiles (adds custom entries to the fan combo)
  reloadFanProfiles();
  fprintf( log2, "Reloaded fan profiles from ProfileManager\n" );
  fflush( log2 );

  // If we were in the middle of saving, mark as complete
  if ( m_saveInProgress )
  {
    m_saveInProgress = false;
    m_profileChanged = false;
    statusBar()->showMessage( "Profile saved successfully" );
    updateButtonStates();
  }

  // Ensure buttons reflect current profile set (remove button availability etc.)
  updateButtonStates();

  fprintf( log2, "Profiles updated. Count: %ld\n", (long)m_profileManager->allProfiles().size() );
  fflush( log2 );
  fclose( log2 );
}

void MainWindow::onActiveProfileIndexChanged()
{
  int activeIndex = m_profileManager->activeProfileIndex();
  if ( m_profileCombo->currentIndex() != activeIndex )
  {
    m_profileCombo->blockSignals( true );
    m_profileCombo->setCurrentIndex( activeIndex );
    m_profileCombo->blockSignals( false );
  }
  m_selectedProfileIndex = activeIndex;
}

// Profile detail control slot implementations
void MainWindow::onBrightnessSliderChanged( int value )
{
  m_brightnessValueLabel->setText( QString::number( value ) + "%" );
}

void MainWindow::onMinFanSpeedChanged( int value )
{
  m_minFanSpeedValue->setText( QString::number( value ) + "%" );
}

void MainWindow::onMaxFanSpeedChanged( int value )
{
  m_maxFanSpeedValue->setText( QString::number( value ) + "%" );
}

void MainWindow::onOffsetFanSpeedChanged( int value )
{
  m_offsetFanSpeedValue->setText( QString::number( value ) + "%" );
}

void MainWindow::onCpuCoresChanged( int value )
{
  m_cpuCoresValue->setText( QString::number( value ) );
}

void MainWindow::onMaxFrequencyChanged( int value )
{
  double freqGHz = value / 1000.0;
  m_maxFrequencyValue->setText( QString::number( freqGHz, 'f', 1 ) + " GHz" );
}

void MainWindow::onODMPowerLimit1Changed( int value )
{
  m_odmPowerLimit1Value->setText( QString::number( value ) + " W" );
}

void MainWindow::onODMPowerLimit2Changed( int value )
{
  m_odmPowerLimit2Value->setText( QString::number( value ) + " W" );
}

void MainWindow::onODMPowerLimit3Changed( int value )
{
  m_odmPowerLimit3Value->setText( QString::number( value ) + " W" );
}

void MainWindow::onGpuPowerChanged( int value )
{
  m_gpuPowerValue->setText( QString::number( value ) + " W" );
}

void MainWindow::loadProfileDetails( const QString &profileName )
{
  // Reset change flag when loading a new profile
  m_profileChanged = false;
  m_currentLoadedProfile = profileName;
  updateButtonStates();
  
  FILE *log = fopen( "/tmp/ucc-debug.log", "a" );
  fprintf( log, "\n=== loadProfileDetails called with: %s\n", profileName.toStdString().c_str() );
  fflush( log );
  
  // Get the profile ID from the profile name
  QString profileId = m_profileManager->getProfileIdByName( profileName );
  

  if ( profileId.isEmpty() )
  {
    fprintf( log, "ERROR: Failed to find profile ID for %s\n", profileName.toStdString().c_str() );
    fflush( log );
    fclose( log );
    return;
  }
  fprintf( log, "Profile ID found: %s\n", profileId.toStdString().c_str() );
  fflush( log );

  // Get the profile JSON from ProfileManager using the profile ID
  QString profileJson = m_profileManager->getProfileDetails( profileId );
  

  if ( profileJson.isEmpty() )
  {
    fprintf( log, "ERROR: Failed to load profile data for %s\n", profileName.toStdString().c_str() );
    fflush( log );
    fclose( log );
    return;
  }
  fprintf( log, "Profile JSON length: %ld\n", (long)profileJson.length() );
  fprintf( log, "First 300 chars: %s\n", profileJson.left( 300 ).toStdString().c_str() );
  fflush( log );

  QJsonDocument doc = QJsonDocument::fromJson( profileJson.toUtf8() );

  if ( !doc.isObject() )
  {
    fprintf( log, "ERROR: Invalid profile JSON structure\n" );
    fflush( log );
    fclose( log );
    return;
  }

  QJsonObject obj = doc.object();
  fprintf( log, "Profile object keys: " );
  for ( const auto &key : obj.keys() )
  {
    fprintf( log, "%s ", key.toStdString().c_str() );
  }
  fprintf( log, "\n" );
  fflush( log );

  // Block signals while updating to avoid triggering slot updates
  m_brightnessSlider->blockSignals( true );
  m_setBrightnessCheckBox->blockSignals( true );
  m_profileFanProfileCombo->blockSignals( true );
  m_fanProfileCombo->blockSignals( true );
  m_minFanSpeedSlider->blockSignals( true );
  m_maxFanSpeedSlider->blockSignals( true );
  m_offsetFanSpeedSlider->blockSignals( true );
  m_cpuCoresSlider->blockSignals( true );
  m_governorCombo->blockSignals( true );
  m_minFrequencySlider->blockSignals( true );
  m_maxFrequencySlider->blockSignals( true );
  m_odmPowerLimit1Slider->blockSignals( true );
  m_odmPowerLimit2Slider->blockSignals( true );
  m_odmPowerLimit3Slider->blockSignals( true );
  m_gpuPowerSlider->blockSignals( true );
  m_profileKeyboardProfileCombo->blockSignals( true );
  m_mainsButton->blockSignals( true );
  m_batteryButton->blockSignals( true );

  // Set profile name and description fields
  if ( obj.contains( "name" ) ) {
    QString name = obj["name"].toString();
    if ( m_nameEdit ) {
      m_nameEdit->blockSignals( true );
      m_nameEdit->setText( name );
      m_nameEdit->blockSignals( false );
    }
  }

  // Load Display settings (nested in display object)

  if ( obj.contains( "display" ) && obj["display"].isObject() )
  {
    QJsonObject displayObj = obj["display"].toObject();
    

    if ( displayObj.contains( "brightness" ) )
    {
      int brightness = displayObj["brightness"].toInt( 100 );
      m_brightnessSlider->setValue( brightness );
    }
    

    if ( displayObj.contains( "useBrightness" ) )
    {
      bool useBrightness = displayObj["useBrightness"].toBool( false );
      m_setBrightnessCheckBox->setChecked( useBrightness );
    }
  }

  // Load Fan Control settings (nested in fan object)

  QString loadedFanProfile;
  if ( obj.contains( "fan" ) && obj["fan"].isObject() )
  {
    QJsonObject fanObj = obj["fan"].toObject();
    

    if ( fanObj.contains( "fanProfile" ) )
    {
      QString fanProfile = fanObj["fanProfile"].toString( "Balanced" );
      int idx = m_profileFanProfileCombo->findText( fanProfile );

      if ( idx >= 0 )
      {
        m_profileFanProfileCombo->setCurrentIndex( idx );
        m_fanProfileCombo->setCurrentIndex( idx );
        loadedFanProfile = fanProfile;
      }
    }

    if ( fanObj.contains( "minimumFanspeed" ) )
      m_minFanSpeedSlider->setValue( fanObj["minimumFanspeed"].toInt( 0 ) );

    if ( fanObj.contains( "maximumFanspeed" ) )
      m_maxFanSpeedSlider->setValue( fanObj["maximumFanspeed"].toInt( 70 ) );

    if ( fanObj.contains( "offsetFanspeed" ) )
      m_offsetFanSpeedSlider->setValue( fanObj["offsetFanspeed"].toInt( 0 ) );

    // Load sameSpeed from profile (default true)
    if ( fanObj.contains( "sameSpeed" ) )
      m_sameFanSpeedCheckBox->setChecked( fanObj["sameSpeed"].toBool( true ) );
    else
      m_sameFanSpeedCheckBox->setChecked( true );
  }

  // Load CPU settings (nested in cpu object)

  if ( obj.contains( "cpu" ) && obj["cpu"].isObject() )
  {
    QJsonObject cpuObj = obj["cpu"].toObject();
    

    if ( cpuObj.contains( "onlineCores" ) )
      m_cpuCoresSlider->setValue( cpuObj["onlineCores"].toInt( 32 ) );

    if ( cpuObj.contains( "governor" ) )
    {
      QString governor = cpuObj["governor"].toString();
      int index = m_governorCombo->findData( governor );
      if ( index >= 0 )
        m_governorCombo->setCurrentIndex( index );
      else
        m_governorCombo->setCurrentIndex( 0 ); // default to first
    }
    else if ( cpuObj.contains( "useMaxPerfGov" ) )
    {
      // backward compatibility
      bool useMaxPerf = cpuObj["useMaxPerfGov"].toBool( true );
      QString governor = useMaxPerf ? "performance" : "powersave";
      int index = m_governorCombo->findData( governor );
      if ( index >= 0 )
        m_governorCombo->setCurrentIndex( index );
      else
        m_governorCombo->setCurrentIndex( 0 );
    }

    if ( cpuObj.contains( "scalingMinFrequency" ) )
      m_minFrequencySlider->setValue( cpuObj["scalingMinFrequency"].toInt( 500 ) );

    if ( cpuObj.contains( "scalingMaxFrequency" ) )
      m_maxFrequencySlider->setValue( cpuObj["scalingMaxFrequency"].toInt( 5500 ) );
  }

  // Load ODM Power Limits (TDP) settings (nested in odmPowerLimits object)
  // First, set slider ranges from hardware limits
  std::vector< int > hardwareLimits = m_profileManager->getHardwarePowerLimits();
  fprintf( log, "Hardware limits size: %zu\n", hardwareLimits.size() );
  for ( size_t i = 0; i < hardwareLimits.size(); ++i )
  {
    fprintf( log, "  Hardware limit %zu = %d\n", i, hardwareLimits[i] );
  }
  fflush( log );
  

  if ( hardwareLimits.size() > 0 )
  {
    m_odmPowerLimit1Slider->setMaximum( hardwareLimits[0] );
    fprintf( log, "Set ODM Power Limit 1 max to: %d\n", hardwareLimits[0] );
  }

  if ( hardwareLimits.size() > 1 )
  {
    m_odmPowerLimit2Slider->setMaximum( hardwareLimits[1] );
    fprintf( log, "Set ODM Power Limit 2 max to: %d\n", hardwareLimits[1] );
  }

  if ( hardwareLimits.size() > 2 )
  {
    m_odmPowerLimit3Slider->setMaximum( hardwareLimits[2] );
    fprintf( log, "Set ODM Power Limit 3 max to: %d\n", hardwareLimits[2] );
  }
  fflush( log );
  
  // Set GPU power max from hardware
  if ( auto gpuMax = m_profileManager->getClient()->getNVIDIAPowerCTRLMaxPowerLimit() )
  {
    m_gpuPowerSlider->setMaximum( *gpuMax );
    fprintf( log, "Set GPU power max to: %d\n", *gpuMax );
    fflush( log );
  }
  
  // Then, set slider values from profile

  if ( obj.contains( "odmPowerLimits" ) && obj["odmPowerLimits"].isObject() )
  {
    QJsonObject odmLimitsObj = obj["odmPowerLimits"].toObject();
    fprintf( log, "odmPowerLimits object found\n" );
    fflush( log );
    

    if ( odmLimitsObj.contains( "tdpValues" ) && odmLimitsObj["tdpValues"].isArray() )
    {
      QJsonArray tdpArray = odmLimitsObj["tdpValues"].toArray();
      fprintf( log, "tdpValues array found with size: %ld\n", (long)tdpArray.size() );
      
      // Load actual values from profile - these are the current settings

      if ( tdpArray.size() > 0 )
      {
        int val0 = tdpArray[0].toInt();
        fprintf( log, "Setting ODM Power Limit 1 to: %d\n", val0 );
        m_odmPowerLimit1Slider->setValue( val0 );
      }
      

      if ( tdpArray.size() > 1 )
      {
        int val1 = tdpArray[1].toInt();
        fprintf( log, "Setting ODM Power Limit 2 to: %d\n", val1 );
        m_odmPowerLimit2Slider->setValue( val1 );
      }
      

      if ( tdpArray.size() > 2 )
      {
        int val2 = tdpArray[2].toInt();
        fprintf( log, "Setting ODM Power Limit 3 to: %d\n", val2 );
        m_odmPowerLimit3Slider->setValue( val2 );
      }
    }
    else
    {
      fprintf( log, "WARNING: tdpValues not found or not an array in odmPowerLimits\n" );
    }
  }
  else
  {
    fprintf( log, "WARNING: odmPowerLimits object not found in profile JSON\n" );
  }
  fflush( log );

  // Load NVIDIA Power Control settings (nested in nvidiaPowerCTRLProfile object)

  if ( obj.contains( "nvidiaPowerCTRLProfile" ) && obj["nvidiaPowerCTRLProfile"].isObject() )
  {
    QJsonObject gpuObj = obj["nvidiaPowerCTRLProfile"].toObject();
    

    if ( gpuObj.contains( "cTGPOffset" ) )
      m_gpuPowerSlider->setValue( gpuObj["cTGPOffset"].toInt( 175 ) + 100 ); // Offset value, adjust as needed
  }

  // Load Keyboard settings (nested in keyboard object) - reference by name only
  if ( obj.contains( "keyboard" ) && obj["keyboard"].isObject() )
  {
    QJsonObject keyboardObj = obj["keyboard"].toObject();
    if ( keyboardObj.contains( "profile" ) )
    {
      QString keyboardProfile = keyboardObj["profile"].toString();
      int idx = m_profileKeyboardProfileCombo->findText( keyboardProfile );
      if ( idx >= 0 )
      {
        m_profileKeyboardProfileCombo->setCurrentIndex( idx );
      }
    }
  }

  // Load power state activation settings
  QString settingsJson = m_profileManager->getSettingsJSON();
  if ( !settingsJson.isEmpty() )
  {
    QJsonDocument settingsDoc = QJsonDocument::fromJson( settingsJson.toUtf8() );
    if ( settingsDoc.isObject() )
    {
      QJsonObject settingsObj = settingsDoc.object();
      if ( settingsObj.contains( "stateMap" ) && settingsObj["stateMap"].isObject() )
      {
        QJsonObject stateMap = settingsObj["stateMap"].toObject();
        QString mainsProfile = stateMap["power_ac"].toString();
        QString batteryProfile = stateMap["power_bat"].toString();
        
        m_mainsButton->setChecked( mainsProfile == profileId );
        m_batteryButton->setChecked( batteryProfile == profileId );
        
        // Store the loaded power state assignments
        m_loadedMainsAssignment = (mainsProfile == profileId);
        m_loadedBatteryAssignment = (batteryProfile == profileId);
      }
    }
  }

  // Unblock signals
  m_brightnessSlider->blockSignals( false );
  m_setBrightnessCheckBox->blockSignals( false );
  m_profileFanProfileCombo->blockSignals( false );
  m_fanProfileCombo->blockSignals( false );
  m_minFanSpeedSlider->blockSignals( false );
  m_maxFanSpeedSlider->blockSignals( false );
  m_offsetFanSpeedSlider->blockSignals( false );
  m_cpuCoresSlider->blockSignals( false );
  m_governorCombo->blockSignals( false );
  m_minFrequencySlider->blockSignals( false );
  m_maxFrequencySlider->blockSignals( false );
  m_odmPowerLimit1Slider->blockSignals( false );
  m_odmPowerLimit2Slider->blockSignals( false );
  m_odmPowerLimit3Slider->blockSignals( false );
  m_gpuPowerSlider->blockSignals( false );
  m_profileKeyboardProfileCombo->blockSignals( false );
  m_mainsButton->blockSignals( false );
  m_batteryButton->blockSignals( false );

  // Trigger label updates by calling the slots directly
  onBrightnessSliderChanged( m_brightnessSlider->value() );
  onMinFanSpeedChanged( m_minFanSpeedSlider->value() );
  onMaxFanSpeedChanged( m_maxFanSpeedSlider->value() );
  onOffsetFanSpeedChanged( m_offsetFanSpeedSlider->value() );
  onCpuCoresChanged( m_cpuCoresSlider->value() );
  onMaxFrequencyChanged( m_maxFrequencySlider->value() );
  onODMPowerLimit1Changed( m_odmPowerLimit1Slider->value() );
  onODMPowerLimit2Changed( m_odmPowerLimit2Slider->value() );
  onODMPowerLimit3Changed( m_odmPowerLimit3Slider->value() );
  onGpuPowerChanged( m_gpuPowerSlider->value() );
  
  // Trigger fan profile change if one was loaded
  if ( !loadedFanProfile.isEmpty() )
  {
    onFanProfileChanged( loadedFanProfile );
  }
  
  fprintf( log, "Profile details loaded for: %s\n", profileName.toStdString().c_str() );
  fprintf( log, "Final slider values: ODM1=%d, ODM2=%d, ODM3=%d\n", 
           m_odmPowerLimit1Slider->value(), m_odmPowerLimit2Slider->value(), m_odmPowerLimit3Slider->value() );
  fprintf( log, "Final slider maxima: ODM1_max=%d, ODM2_max=%d, ODM3_max=%d\n",
           m_odmPowerLimit1Slider->maximum(), m_odmPowerLimit2Slider->maximum(), m_odmPowerLimit3Slider->maximum() );
  fflush( log );
  
  // Enable/disable editing widgets based on whether profile is custom
  const bool isCustom = m_profileManager ? m_profileManager->isCustomProfile( profileName ) : false;
  updateProfileEditingWidgets( isCustom );

  fclose( log );
}

void MainWindow::updateProfileEditingWidgets( bool isCustom )
{
  // Enable/disable editing widgets based on whether profile is custom
  
  // Description edit
  if ( m_descriptionEdit ) {
    m_descriptionEdit->setEnabled( isCustom );
    m_descriptionEdit->setReadOnly( !isCustom );
  }

  // Name edit
  if ( m_nameEdit ) {
    m_nameEdit->setEnabled( isCustom );
    m_nameEdit->setReadOnly( !isCustom );
  }
  
  // Auto-activate buttons (always enabled for power state assignment)
  if ( m_mainsButton ) m_mainsButton->setEnabled( true );
  if ( m_batteryButton ) m_batteryButton->setEnabled( true );
  
  // Display controls
  if ( m_setBrightnessCheckBox ) m_setBrightnessCheckBox->setEnabled( isCustom );
  if ( m_brightnessSlider ) m_brightnessSlider->setEnabled( isCustom );
  
  // Fan controls
  if ( m_profileFanProfileCombo ) m_profileFanProfileCombo->setEnabled( isCustom );
  if ( m_minFanSpeedSlider ) m_minFanSpeedSlider->setEnabled( isCustom );
  if ( m_maxFanSpeedSlider ) m_maxFanSpeedSlider->setEnabled( isCustom );
  if ( m_offsetFanSpeedSlider ) m_offsetFanSpeedSlider->setEnabled( isCustom );
  if ( m_sameFanSpeedCheckBox ) m_sameFanSpeedCheckBox->setEnabled( isCustom );
  
  // CPU controls
  if ( m_cpuCoresSlider ) m_cpuCoresSlider->setEnabled( isCustom );
  if ( m_governorCombo ) m_governorCombo->setEnabled( isCustom );
  if ( m_minFrequencySlider ) m_minFrequencySlider->setEnabled( isCustom );
  if ( m_maxFrequencySlider ) m_maxFrequencySlider->setEnabled( isCustom );
  
  // ODM Power controls
  if ( m_odmPowerLimit1Slider ) m_odmPowerLimit1Slider->setEnabled( isCustom );
  if ( m_odmPowerLimit2Slider ) m_odmPowerLimit2Slider->setEnabled( isCustom );
  if ( m_odmPowerLimit3Slider ) m_odmPowerLimit3Slider->setEnabled( isCustom );
  
  // GPU controls
  if ( m_gpuPowerSlider ) m_gpuPowerSlider->setEnabled( isCustom );
}

void MainWindow::markChanged()
{
  m_profileChanged = true;
  updateButtonStates();
}

void MainWindow::updateButtonStates( void)
{
  // Update profile page buttons if available
  if ( profileTopWidgetsAvailable() )
  {
    m_removeProfileButton->setEnabled( m_profileManager->isCustomProfile( m_profileCombo->currentText() ) );
  }

  // Update fan profile controls individually so the Copy button can be updated
  // even if optional buttons (like Revert) are not added.
  if ( m_fanProfileCombo )
  {
    const QString fanProfileName = m_fanProfileCombo->currentText();
    const bool isCustomFanProfile = ( not fanProfileName.isEmpty() and not m_builtinFanProfiles.contains( fanProfileName ) );

    if ( m_applyFanProfilesButton )
      m_applyFanProfilesButton->setEnabled( m_UccdClient->isConnected() );

    if ( m_saveFanProfilesButton )
      m_saveFanProfilesButton->setEnabled( isCustomFanProfile );

    // Allow copying any saved profile (built-in or custom) — enable when a profile is selected
    if ( m_copyFanProfileButton )
      m_copyFanProfileButton->setEnabled( not fanProfileName.isEmpty() );

    if ( m_revertFanProfilesButton )
      m_revertFanProfilesButton->setEnabled( isCustomFanProfile && m_UccdClient->isConnected() );
  }
}

void MainWindow::onApplyClicked()
{
  if ( m_selectedProfileIndex >= 0 )
  {
    QString profileName = m_profileCombo->currentText();
    m_profileManager->setActiveProfileByIndex( m_selectedProfileIndex );
    m_applyButton->setEnabled( false );
    statusBar()->showMessage( "Profile applied: " + profileName );
  }
  else
  {
    statusBar()->showMessage( "No profile selected" );
  }
}

void MainWindow::onSaveClicked()
{
  QString profileName = m_profileCombo->currentText();
  QString profileId = m_profileManager->getProfileIdByName( profileName );
  const bool isCustom = m_profileManager->isCustomProfile( profileName );
  
  if ( isCustom )
  {
    // Save full profile changes for custom profiles
    // Build updated profile JSON
    QJsonObject profileObj;
    profileObj["id"] = profileId;
    // Use the possibly edited name from the name edit field
    if ( m_nameEdit )
      profileObj["name"] = m_nameEdit->text();
    else
      profileObj["name"] = profileName;
    profileObj["description"] = m_descriptionEdit->toPlainText();
    
    // Brightness settings
    QJsonObject displayObj;
    if ( m_setBrightnessCheckBox->isChecked() )
    {
      displayObj["brightness"] = m_brightnessSlider->value();
    }
    profileObj["display"] = displayObj;
    
    // Fan settings
    QJsonObject fanObj;
    fanObj["fanProfile"] = m_profileFanProfileCombo->currentText();
    fanObj["minSpeed"] = m_minFanSpeedSlider->value();
    fanObj["maxSpeed"] = m_maxFanSpeedSlider->value();
    fanObj["offset"] = m_offsetFanSpeedSlider->value();
    // Persist same-speed setting
    fanObj["sameSpeed"] = m_sameFanSpeedCheckBox ? m_sameFanSpeedCheckBox->isChecked() : true;

    profileObj["fan"] = fanObj;

    // CPU settings
    QJsonObject cpuObj;
    cpuObj["cores"] = m_cpuCoresSlider->value();
    cpuObj["governor"] = m_governorCombo->currentData().toString();
    cpuObj["minFrequency"] = m_minFrequencySlider->value();
    cpuObj["maxFrequency"] = m_maxFrequencySlider->value();
    profileObj["cpu"] = cpuObj;
  
  // ODM Power Limit (TDP) settings
  QJsonObject odmObj;
  QJsonArray tdpArray;
  tdpArray.append( m_odmPowerLimit1Slider->value() );
  tdpArray.append( m_odmPowerLimit2Slider->value() );
  tdpArray.append( m_odmPowerLimit3Slider->value() );
  odmObj["tdpValues"] = tdpArray;
  profileObj["odmPowerLimits"] = odmObj;
  
  // GPU settings
  QJsonObject gpuObj;
  gpuObj["cTGPOffset"] = m_gpuPowerSlider->value() - 100; // Reverse the offset
  QJsonObject nvidiaPowerObj;
  nvidiaPowerObj["cTGPOffset"] = gpuObj["cTGPOffset"];
  profileObj["nvidiaPowerCTRLProfile"] = nvidiaPowerObj;

  // Keyboard settings - save reference by name only
  QJsonObject keyboardObj;
  keyboardObj["profile"] = m_profileKeyboardProfileCombo->currentText();
  profileObj["keyboard"] = keyboardObj;

  // Convert to JSON string and save
  QJsonDocument doc( profileObj );
  QString profileJSON = QString::fromUtf8( doc.toJson() );

  m_profileManager->saveProfile( profileJSON );
  }
  else if ( !isCustom && m_profileChanged )
  {
    // User edited a built-in profile; create a custom copy and save the changes
    QString baseName = profileName;
    QString newName;
    int number = 1;
    do {
      newName = QString( "%1 %2" ).arg( baseName ).arg( number );
      number++;
    } while ( m_profileManager->allProfiles().contains( newName ) );

    QString newJson = m_profileManager->createProfileFromDefault( newName );
    if ( newJson.isEmpty() ) {
      QMessageBox::warning( this, "Error", "Failed to create editable copy of profile." );
    } else {
      QJsonDocument doc = QJsonDocument::fromJson( newJson.toUtf8() );
      if ( doc.isObject() ) {
        QJsonObject obj = doc.object();
        // Apply the same fields as for custom save
        QJsonObject displayObj;
        if ( m_setBrightnessCheckBox->isChecked() ) displayObj["brightness"] = m_brightnessSlider->value();
        obj["display"] = displayObj;

        QJsonObject fanObj;
        fanObj["fanProfile"] = m_profileFanProfileCombo->currentText();
        fanObj["minSpeed"] = m_minFanSpeedSlider->value();
        fanObj["maxSpeed"] = m_maxFanSpeedSlider->value();
        fanObj["offset"] = m_offsetFanSpeedSlider->value();
        fanObj["sameSpeed"] = m_sameFanSpeedCheckBox ? m_sameFanSpeedCheckBox->isChecked() : true;
        obj["fan"] = fanObj;

        QJsonObject cpuObj;
        cpuObj["cores"] = m_cpuCoresSlider->value();
        cpuObj["governor"] = m_governorCombo->currentData().toString();
        cpuObj["minFrequency"] = m_minFrequencySlider->value();
        cpuObj["maxFrequency"] = m_maxFrequencySlider->value();
        obj["cpu"] = cpuObj;

        QJsonObject keyboardObj;
        keyboardObj["profile"] = m_profileKeyboardProfileCombo->currentText();
        obj["keyboard"] = keyboardObj;

        QJsonDocument out( obj );
        m_profileManager->saveProfile( QString::fromUtf8( out.toJson( QJsonDocument::Compact ) ) );

        // Switch to new profile and update profileId for stateMap
        int idx = m_profileCombo->findText( newName );
        if ( idx != -1 ) {
          m_profileCombo->setCurrentIndex( idx );
          profileId = m_profileManager->getProfileIdByName( newName );
        }
      }
    }
  }

  // For both custom and built-in profiles, update stateMap based on mains/battery button states
  if ( m_mainsButton->isChecked() )
  {
    m_profileManager->setStateMap( "power_ac", profileId );
  }
  if ( m_batteryButton->isChecked() )
  {
    m_profileManager->setStateMap( "power_bat", profileId );
  }
  
  // Indicate saving; actual success will be reflected when ProfileManager signals
  m_saveInProgress = true;
  statusBar()->showMessage( "Saving profile..." );
  updateButtonStates();
}

void MainWindow::onSaveFanProfilesClicked()
{
  // Save fan profiles via DBus
  saveFanPoints();
  statusBar()->showMessage( "Fan profiles saved" );
}

void MainWindow::onAddProfileClicked()
{
  // Generate a unique profile name
  QString baseName = "New Profile";
  QString profileName;
  int counter = 1;
  
  do {
    profileName = QString("%1 %2").arg(baseName).arg(counter);
    counter++;
  } while (m_profileManager->allProfiles().contains(profileName));
  
  // Create profile from default
  QString profileJson = m_profileManager->createProfileFromDefault(profileName);
  if (!profileJson.isEmpty()) {
    statusBar()->showMessage( QString("Profile '%1' created successfully").arg(profileName) );
    
    // Switch to the newly created profile
    int newIndex = m_profileCombo->findText(profileName);
    if (newIndex != -1) {
      m_profileCombo->setCurrentIndex(newIndex);
    }
  }
  else
    QMessageBox::warning(this, "Error", "Failed to create new profile.");
}
void MainWindow::onCopyProfileClicked()
{
  QString current = m_profileCombo->currentText();
  
  // Allow copying any profile (built-in or custom)
  QString profileId = m_profileManager->getProfileIdByName(current);
  QString json = m_profileManager->getProfileDetails(profileId);
  if (json.isEmpty()) {
    QMessageBox::warning(this, "Error", "Failed to get profile data.");
    return;
  }
  
  QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
  if (!doc.isObject()) {
    QMessageBox::warning(this, "Error", "Invalid profile data.");
    return;
  }
  
  QJsonObject obj = doc.object();
  
  // Generate a new unique ID for the copied profile
  obj["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
  
  // Generate new name by incrementing number
  QString baseName = current;
  int number = 0;
  int lastSpace = current.lastIndexOf(' ');
  if (lastSpace > 0) {
    QString after = current.mid(lastSpace + 1);
    bool ok;
    int num = after.toInt(&ok);
    if (ok) {
      baseName = current.left(lastSpace);
      number = num;
    }
  }
  QString newName;
  do {
    number++;
    newName = QString("%1 %2").arg(baseName).arg(number);
  } while (m_profileManager->allProfiles().contains(newName));
  
  // Set new name
  obj["name"] = newName;
  
  // Save
  QString newJson = QJsonDocument(obj).toJson(QJsonDocument::Compact);
  m_profileManager->saveProfile(newJson);
  
  // Switch
  int newIndex = m_profileCombo->findText(newName);
  if (newIndex != -1) {
    m_profileCombo->setCurrentIndex(newIndex);
  }
  
  statusBar()->showMessage( QString("Profile '%1' copied to '%2'").arg(current).arg(newName) );
}

void MainWindow::onRemoveProfileClicked()
{
  QString currentProfile = m_profileCombo->currentText();
  
  // Check if it's a built-in profile
  if (!m_profileManager->isCustomProfile(currentProfile)) {
    QMessageBox::information(this, "Cannot Remove", 
                            "Built-in profiles cannot be removed.");
    return;
  }
  
  // Confirm deletion
  QMessageBox::StandardButton reply = QMessageBox::question(
    this, "Remove Profile",
    QString("Are you sure you want to remove the profile '%1'?").arg(currentProfile),
    QMessageBox::Yes | QMessageBox::No
  );
  
  if (reply == QMessageBox::Yes) {
    QString profileId = m_profileManager->getProfileIdByName(currentProfile);
    m_profileManager->deleteProfile(profileId);
    statusBar()->showMessage( QString("Profile '%1' removed").arg(currentProfile) );
  }
}

void MainWindow::onAddFanProfileClicked()
{
  // Generate a unique fan profile name
  QString baseName = "Custom Fan Profile";
  QString profileName;
  int counter = 1;
  
  do {
    profileName = QString("%1 %2").arg(baseName).arg(counter);
    counter++;
  } while (m_fanProfileCombo->findText(profileName) != -1);
  
  // Add to combo
  m_fanProfileCombo->addItem(profileName);
  m_fanProfileCombo->setCurrentText(profileName);
  statusBar()->showMessage( QString("Fan profile '%1' created").arg(profileName) );
}

void MainWindow::onRemoveFanProfileClicked()
{
  QString currentProfile = m_fanProfileCombo->currentText();
  
  // Check if it's a built-in profile
  if ( m_builtinFanProfiles.contains( currentProfile ) ) {
    QMessageBox::information(this, "Cannot Remove", 
                            "Built-in fan profiles cannot be removed.");
    return;
  }
  
  // Confirm deletion
  QMessageBox::StandardButton reply = QMessageBox::question(
    this, "Remove Fan Profile",
    QString("Are you sure you want to remove the fan profile '%1'?").arg(currentProfile),
    QMessageBox::Yes | QMessageBox::No
  );
  
  if (reply == QMessageBox::Yes) {
    // Remove from persistent storage and UI
    if ( m_profileManager->deleteFanProfile( currentProfile ) ) {
      // Remove from both fan profile lists
      int idx = m_fanProfileCombo->currentIndex();
      if ( idx >= 0 ) m_fanProfileCombo->removeItem( idx );
      if ( m_profileFanProfileCombo ) {
        int idx2 = m_profileFanProfileCombo->findText( currentProfile );
        if ( idx2 != -1 ) m_profileFanProfileCombo->removeItem( idx2 );
      }
      statusBar()->showMessage( QString("Fan profile '%1' removed").arg(currentProfile) );
    } else {
      QMessageBox::warning(this, "Remove Failed", "Failed to remove custom fan profile.");
    }
  }
}

void MainWindow::onFanProfileChanged(const QString& profileName)
{
  Q_UNUSED(profileName);

  QString json = m_profileManager->getFanProfile(profileName);
  QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
  if (doc.isObject()) {
    QJsonObject obj = doc.object();
    
    // Load CPU points
    if (obj.contains("tableCPU")) {
      QJsonArray arr = obj["tableCPU"].toArray();
      QVector<FanCurveEditorWidget::Point> cpuPoints;
      for (const QJsonValue &v : arr) {
        QJsonObject o = v.toObject();
        double temp = o["temp"].toDouble();
        double speed = o["speed"].toDouble();
        cpuPoints.append({temp, speed});
      }
      if (m_cpuFanCurveEditor && !cpuPoints.isEmpty()) {
        m_cpuFanCurveEditor->setPoints(cpuPoints);
      }
    }
    
    // Load GPU points
    if (obj.contains("tableGPU")) {
      QJsonArray arr = obj["tableGPU"].toArray();
      QVector<FanCurveEditorWidget::Point> gpuPoints;
      for (const QJsonValue &v : arr) {
        QJsonObject o = v.toObject();
        double temp = o["temp"].toDouble();
        double speed = o["speed"].toDouble();
        gpuPoints.append({temp, speed});
      }
      if (m_gpuFanCurveEditor && !gpuPoints.isEmpty()) {
        m_gpuFanCurveEditor->setPoints(gpuPoints);
      }
    }
  }
  
  // Update the current profile selection
  m_currentFanProfile = profileName;
  
  // Synchronize profile tab fan profile combo with fan tab selection
  m_profileFanProfileCombo->blockSignals(true);
  int idx = m_profileFanProfileCombo->findText(profileName);
  if (idx >= 0) {
    m_profileFanProfileCombo->setCurrentIndex(idx);
  }
  m_profileFanProfileCombo->blockSignals(false);
  
  // Set editors editable only for custom profiles (those not in built-ins)
  bool isEditable = !m_builtinFanProfiles.contains( profileName );
  if (m_cpuFanCurveEditor) {
    m_cpuFanCurveEditor->setEditable(isEditable);
  }
  if (m_gpuFanCurveEditor) {
    m_gpuFanCurveEditor->setEditable(isEditable);
  }
  
  // Update button states
  updateButtonStates();
}

void MainWindow::onCpuFanPointsChanged(const QVector<FanCurveEditorWidget::Point>& points)
{
  m_cpuFanPoints.clear();
  for (const auto& p : points) {
    m_cpuFanPoints.append({static_cast<int>(p.temp), static_cast<int>(p.duty)});
  }
}

void MainWindow::reloadFanProfiles()
{
  qDebug() << "Reloading fan profiles from daemon and custom profiles";
  qDebug() << "UccdClient connected:" << (m_UccdClient ? m_UccdClient->isConnected() : false);

  // Preserve current selection
  QString prev = m_fanProfileCombo ? m_fanProfileCombo->currentText() : QString();

  if ( m_fanProfileCombo )
    m_fanProfileCombo->clear();

  m_builtinFanProfiles.clear();

  // Load built-in names from daemon (if available)
  if ( m_UccdClient )
  {
    if ( auto names = m_UccdClient->getFanProfileNames() )
    {
      FILE *log = fopen("/tmp/ucc-debug.log","a");
      fprintf(log, "Found %zu built-in fan profiles from daemon:\n", names->size());
      for ( const auto &n : *names ) {
        QString qn = QString::fromStdString( n );
        m_fanProfileCombo->addItem( qn );
        m_builtinFanProfiles.append( qn );
        fprintf(log, "  %s\n", n.c_str());
      }
      fclose(log);
    }
    else
    {
      FILE *log = fopen("/tmp/ucc-debug.log","a");
      fprintf(log, "No built-in fan profiles found from daemon at this time\n");
      fclose(log);
    }
  }

  // Append persisted custom fan profiles
  for ( const auto &name : m_profileManager->customFanProfiles() )
  {
    if ( m_fanProfileCombo->findText( name ) == -1 )
      m_fanProfileCombo->addItem( name );
  }

  // Mirror into profile page combo if present
  if ( m_profileFanProfileCombo )
  {
    m_profileFanProfileCombo->blockSignals(true);
    m_profileFanProfileCombo->clear();
    for ( int i = 0; i < m_fanProfileCombo->count(); ++i )
      m_profileFanProfileCombo->addItem( m_fanProfileCombo->itemText(i) );
    m_profileFanProfileCombo->blockSignals(false);
  }

  // Restore selection or pick first
  if ( !prev.isEmpty() && m_fanProfileCombo->findText(prev) != -1 )
    m_fanProfileCombo->setCurrentText(prev);
  else if ( m_fanProfileCombo->count() > 0 )
    m_fanProfileCombo->setCurrentIndex(0);

  // Trigger change handler to update editors/buttons
  if ( m_fanProfileCombo && m_fanProfileCombo->count() > 0 )
    onFanProfileChanged( m_fanProfileCombo->currentText() );
  else
    updateButtonStates();
}



void MainWindow::onUccdConnectionChanged( bool connected )
{
  qDebug() << "Uccd connection status changed:" << connected;
  if ( connected )
    reloadFanProfiles();
}

void MainWindow::onGpuFanPointsChanged(const QVector<FanCurveEditorWidget::Point>& points)
{
  m_gpuFanPoints.clear();
  for (const auto& p : points) {
    m_gpuFanPoints.append({static_cast<int>(p.temp), static_cast<int>(p.duty)});
  }
}

void MainWindow::onCopyFanProfileClicked()
{
  QString currentProfile = m_fanProfileCombo->currentText();
  if ( currentProfile.isEmpty() ) return;

  // Get the current profile data
  QString json = m_profileManager->getFanProfile( currentProfile );
  if ( json.isEmpty() ) {
    QMessageBox::warning(this, "Error", "Failed to get fan profile data.");
    return;
  }

  // Generate a unique custom fan profile name
  QString baseName = "Custom Fan Profile";
  QString profileName;
  int counter = 1;
  do {
    profileName = QString("%1 %2").arg(baseName).arg(counter);
    counter++;
  } while ( m_fanProfileCombo->findText( profileName ) != -1 );

  // Save it under the new name
  if ( m_profileManager->setFanProfile( profileName, json ) ) {
    m_fanProfileCombo->addItem( profileName );
    if ( m_profileFanProfileCombo && m_profileFanProfileCombo->findText( profileName ) == -1 )
      m_profileFanProfileCombo->addItem( profileName );
    m_fanProfileCombo->setCurrentText( profileName );
    statusBar()->showMessage( QString("Copied '%1' profile to '%2'").arg(currentProfile).arg(profileName) );
  }
  else {
    QMessageBox::warning(this, "Error", "Failed to copy profile to new custom profile.");
  }
}

void MainWindow::onApplyFanProfilesClicked()
{
  if ( not m_cpuFanCurveEditor and not m_gpuFanCurveEditor )
  {
    QMessageBox::warning( this, "No Editors", "No fan curve editors available to apply fan profiles." );
    return;
  }

  if ( !m_UccdClient || !m_UccdClient->isConnected() )
  {
    QMessageBox::warning( this, "Not connected", "Not connected to system service; cannot apply fan profiles." );
    return;
  }

  const auto &cpuPoints = m_cpuFanCurveEditor->points();
  const auto &gpuPoints = m_gpuFanCurveEditor->points();
  QJsonObject root;
  QJsonArray cpuArr;
  QJsonArray gpuArr;

  for ( const auto &p : cpuPoints )
  {
    QJsonObject o;
    o["temp"] = p.temp;
    o["speed"] = p.duty;
    cpuArr.append( o );
  }

  for ( const auto &p : gpuPoints )
  {
    QJsonObject o;
    o["temp"] = p.temp;
    o["speed"] = p.duty;
    gpuArr.append( o );
  }

  root[ "cpu" ] = cpuArr;
  root[ "gpu" ] = gpuArr;

  QJsonDocument doc( root );
  QString json = QString::fromUtf8( doc.toJson( QJsonDocument::Compact ) );

  if ( m_UccdClient->applyFanProfiles( json.toStdString() ) )
  {
    statusBar()->showMessage( "Temporary fan profiles applied" );

    // Keep an internal copy so UI actions like revert have the current values
    m_cpuFanPoints.clear();
    for ( const auto &p : m_cpuFanCurveEditor->points() )
      m_cpuFanPoints.append( { static_cast< int >( p.temp ), static_cast< int >( p.duty ) } );

    m_gpuFanPoints.clear();
    for ( const auto &p : m_gpuFanCurveEditor->points() )
      m_gpuFanPoints.append( { static_cast< int >( p.temp ), static_cast< int >( p.duty ) } );
  }
  else
  {
    QMessageBox::warning( this, "Apply Failed", "Failed to apply fan profiles. Check service logs or connection." );
  }
}

void MainWindow::onRevertFanProfilesClicked()
{
  loadFanPoints();
}

void MainWindow::loadFanPoints()
{
  // Load fan profile JSON for the currently selected fan profile (if custom)
  if ( m_currentFanProfile.isEmpty() ) return;

  QString json = m_profileManager->getFanProfile( m_currentFanProfile );
  QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
  if (doc.isObject()) {
    QJsonObject obj = doc.object();
    if (obj.contains("tableCPU")) {
      QJsonArray arr = obj["tableCPU"].toArray();
      m_cpuFanPoints.clear();
      QVector<FanCurveEditorWidget::Point> cpuPoints;
      for (const QJsonValue &v : arr) {
        QJsonObject o = v.toObject();
        int temp = o["temp"].toInt();
        int speed = o["speed"].toInt();
        m_cpuFanPoints.append({temp, speed});
        cpuPoints.append({static_cast<double>(temp), static_cast<double>(speed)});
      }
      if (m_cpuFanCurveEditor) {
        m_cpuFanCurveEditor->setPoints(cpuPoints);
      }
    }
    if (obj.contains("tableGPU")) {
      QJsonArray arr = obj["tableGPU"].toArray();
      m_gpuFanPoints.clear();
      QVector<FanCurveEditorWidget::Point> gpuPoints;
      for (const QJsonValue &v : arr) {
        QJsonObject o = v.toObject();
        int temp = o["temp"].toInt();
        int speed = o["speed"].toInt();
        m_gpuFanPoints.append({temp, speed});
        gpuPoints.append({static_cast<double>(temp), static_cast<double>(speed)});
      }
      if (m_gpuFanCurveEditor) {
        m_gpuFanCurveEditor->setPoints(gpuPoints);
      }
    }
  }
}


void MainWindow::saveFanPoints()
{
  QJsonObject obj;
  
  // Get CPU points from the editor
  QJsonArray cpuArr;
  if (m_cpuFanCurveEditor) {
    const auto& cpuPoints = m_cpuFanCurveEditor->points();
    for (const auto &p : cpuPoints) {
      QJsonObject o;
      o["temp"] = p.temp;
      o["speed"] = p.duty;
      cpuArr.append(o);
    }
  }
  obj["tableCPU"] = cpuArr;
  
  // Get GPU points from the editor
  QJsonArray gpuArr;
  if (m_gpuFanCurveEditor) {
    const auto& gpuPoints = m_gpuFanCurveEditor->points();
    for (const auto &p : gpuPoints) {
      QJsonObject o;
      o["temp"] = p.temp;
      o["speed"] = p.duty;
      gpuArr.append(o);
    }
  }
  obj["tableGPU"] = gpuArr;
  
  QJsonDocument doc(obj);
  QString json = doc.toJson(QJsonDocument::Compact);

  const QString current = m_fanProfileCombo ? m_fanProfileCombo->currentText() : QString();
  if ( current.isEmpty() ) {
    QMessageBox::warning(this, "Save Failed", "No fan profile selected to save to.");
    return;
  }

  if ( m_builtinFanProfiles.contains( current ) ) {
    QMessageBox::warning(this, "Save Failed", "Cannot overwrite built-in fan profile. Copy it to a custom profile first.");
    return;
  }

  // Save into selected custom profile
  m_profileManager->setFanProfile( current, json );
}










} // namespace ucc
