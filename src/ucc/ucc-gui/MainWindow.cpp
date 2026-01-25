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
#include "../libucc-dbus/TccdClient.hpp"

#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>


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
  m_tccdClient = std::make_unique< TccdClient >( this );

  setWindowTitle( "Uniwill Control Center" );
  setGeometry( 100, 100, 900, 700 );

  setupUI();
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
  
  // Initialize current fan profile
  m_currentFanProfile = "Custom";
  
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

  setupDashboardPage();
  setupProfilesPage();

  // Place the Fan Control tab directly after Profiles and rename it
  setupFanControlTab();
  qDebug() << "Calling updateFanTabVisibility during setupUI";
  updateFanTabVisibility(); // Set initial visibility based on current profile
  setupHardwarePage();
}

void MainWindow::setupFanControlTab()
{
  m_fanTab = new QWidget();
  m_fanTab->setStyleSheet( "background-color: #242424; color: #e6e6e6;" );
  QVBoxLayout *mainLayout = new QVBoxLayout( m_fanTab );
  mainLayout->setContentsMargins( 20, 20, 20, 20 );
  mainLayout->setSpacing( 20 );

  // Top controls: profile selection and copy button
  QHBoxLayout *topLayout = new QHBoxLayout();
  QLabel *profileLabel = new QLabel( "Profile:" );
  topLayout->addWidget( profileLabel );
  m_fanProfileCombo = new QComboBox();
  m_fanProfileCombo->addItem( "Silent" );
  m_fanProfileCombo->addItem( "Quiet" );
  m_fanProfileCombo->addItem( "Balanced" );
  m_fanProfileCombo->addItem( "Cool" );
  m_fanProfileCombo->addItem( "Freezy" );
  m_fanProfileCombo->addItem( "Custom" );
  m_fanProfileCombo->setCurrentText( "Custom" );
  topLayout->addWidget( m_fanProfileCombo );
  
  topLayout->addSpacing( 20 );
  
  QPushButton *copyButton = new QPushButton("Copy to Custom");
  copyButton->setStyleSheet(
    "QPushButton { background-color: #FF9800; color: white; border: none; padding: 6px 12px; border-radius: 4px; font-weight: bold; }"
    "QPushButton:hover { background-color: #F57C00; }"
    "QPushButton:pressed { background-color: #EF6C00; }"
    "QPushButton:disabled { background-color: #666666; color: #999999; }"
  );
  connect(copyButton, &QPushButton::clicked, this, &MainWindow::onCopyFanProfileClicked);
  topLayout->addWidget( copyButton );
  
  topLayout->addStretch();
  
  // Action buttons
  m_applyFanProfilesButton = new QPushButton("Apply");
  m_applyFanProfilesButton->setStyleSheet(
    "QPushButton { background-color: #4CAF50; color: white; border: none; padding: 6px 12px; border-radius: 4px; font-weight: bold; }"
    "QPushButton:hover { background-color: #45a049; }"
    "QPushButton:pressed { background-color: #3e8e41; }"
    "QPushButton:disabled { background-color: #666666; color: #999999; }"
  );
  connect(m_applyFanProfilesButton, &QPushButton::clicked, this, &MainWindow::onApplyFanProfilesClicked);
  topLayout->addWidget(m_applyFanProfilesButton);
  
  m_saveFanProfilesButton = new QPushButton("Save");
  m_saveFanProfilesButton->setStyleSheet(
    "QPushButton { background-color: #2196F3; color: white; border: none; padding: 6px 12px; border-radius: 4px; font-weight: bold; }"
    "QPushButton:hover { background-color: #1976D2; }"
    "QPushButton:pressed { background-color: #1565C0; }"
    "QPushButton:disabled { background-color: #666666; color: #999999; }"
  );
  connect(m_saveFanProfilesButton, &QPushButton::clicked, this, &MainWindow::onSaveFanProfilesClicked);
  topLayout->addWidget(m_saveFanProfilesButton);
  
  m_revertFanProfilesButton = new QPushButton("Revert");
  m_revertFanProfilesButton->setStyleSheet(
    "QPushButton { background-color: #FF9800; color: white; border: none; padding: 6px 12px; border-radius: 4px; font-weight: bold; }"
    "QPushButton:hover { background-color: #F57C00; }"
    "QPushButton:pressed { background-color: #EF6C00; }"
    "QPushButton:disabled { background-color: #666666; color: #999999; }"
  );
  connect(m_revertFanProfilesButton, &QPushButton::clicked, this, &MainWindow::onRevertFanProfilesClicked);
  topLayout->addWidget(m_revertFanProfilesButton);
  
  mainLayout->addLayout( topLayout );

  // CPU and GPU fan curve editors on top of each other
  QVBoxLayout *editorsLayout = new QVBoxLayout();
  editorsLayout->setSpacing( 20 );
  
  // CPU fan curve editor
  QVBoxLayout *cpuLayout = new QVBoxLayout();
  QLabel *cpuLabel = new QLabel( "CPU Fan Curve" );
  cpuLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
  cpuLayout->addWidget( cpuLabel );
  m_cpuFanCurveEditor = new FanCurveEditorWidget();
  m_cpuFanCurveEditor->setStyleSheet( "background-color: #1a1a1a; border: 1px solid #555;" );
  cpuLayout->addWidget( m_cpuFanCurveEditor );
  editorsLayout->addLayout( cpuLayout );
  
  // GPU fan curve editor
  QVBoxLayout *gpuLayout = new QVBoxLayout();
  QLabel *gpuLabel = new QLabel( "GPU Fan Curve" );
  gpuLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
  gpuLayout->addWidget( gpuLabel );
  m_gpuFanCurveEditor = new FanCurveEditorWidget();
  m_gpuFanCurveEditor->setStyleSheet( "background-color: #1a1a1a; border: 1px solid #555;" );
  gpuLayout->addWidget( m_gpuFanCurveEditor );
  editorsLayout->addLayout( gpuLayout );
  
  mainLayout->addLayout( editorsLayout );

  // Connect signals
  connect( m_fanProfileCombo, &QComboBox::currentTextChanged,
           this, &MainWindow::onFanProfileChanged );
  connect( m_cpuFanCurveEditor, &FanCurveEditorWidget::pointsChanged,
           this, &MainWindow::onCpuFanPointsChanged );
  connect( m_gpuFanCurveEditor, &FanCurveEditorWidget::pointsChanged,
           this, &MainWindow::onGpuFanPointsChanged );

  mainLayout->addStretch();

  // Load initial data
  onFanProfileChanged("Custom");
}

void MainWindow::updateFanTabVisibility()
{
  qDebug() << "=== updateFanTabVisibility called ===";
  bool isCustom = m_profileManager->isCustomProfile(m_profileManager->activeProfile());
  bool currentlyVisible = (m_tabs->indexOf(m_fanTab) != -1);

  qDebug() << "updateFanTabVisibility: activeProfile=" << m_profileManager->activeProfile()
           << ", isCustom=" << isCustom << ", currentlyVisible=" << currentlyVisible;

  if (isCustom && !currentlyVisible)
  {
    // Add the fan tab at the end
    qDebug() << "Adding fan tab";
    m_tabs->addTab(m_fanTab, "Profile Fan Control");
  }
  else if (!isCustom && currentlyVisible)
  {
    // Remove the fan tab
    qDebug() << "Removing fan tab";
    m_tabs->removeTab(m_tabs->indexOf(m_fanTab));
  }
}

void MainWindow::setupDashboardPage()
{
  QWidget *dashboardWidget = new QWidget();
  dashboardWidget->setStyleSheet( "background-color: #242424; color: #e6e6e6;" );
  QVBoxLayout *layout = new QVBoxLayout( dashboardWidget );
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
  cpuGrid->addWidget( makeGauge( "CPU - Fan", "RPM", m_fanSpeedLabel ), 0, 2 );
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
  gpuGrid->addWidget( makeGauge( "dGPU - Fan", "RPM", m_gpuFanSpeedLabel ), 0, 2 );
  gpuGrid->addWidget( makeGauge( "dGPU - Power", "W", m_gpuPowerLabel ), 0, 3 );
  layout->addLayout( gpuGrid );

  layout->addStretch();

  m_tabs->addTab( dashboardWidget, "Dashboard" );
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
  m_profileCombo->addItems( m_profileManager->allProfiles() );
  m_profileCombo->setCurrentIndex( m_profileManager->activeProfileIndex() );
  m_selectedProfileIndex = m_profileManager->activeProfileIndex();
  m_applyButton = new QPushButton( "Apply" );
  m_applyButton->setMaximumWidth( 80 );
  m_applyButton->setEnabled( false );
  
  m_saveButton = new QPushButton( "Save" );
  m_saveButton->setMaximumWidth( 80 );
  m_saveButton->setEnabled( false );
  
  m_addProfileButton = new QPushButton( "Add" );
  m_addProfileButton->setMaximumWidth( 60 );
  
  m_removeProfileButton = new QPushButton( "Remove" );
  m_removeProfileButton->setMaximumWidth( 70 );
  
  selectLayout->addWidget( selectLabel );
  selectLayout->addWidget( m_profileCombo, 1 );
  selectLayout->addWidget( m_applyButton );
  selectLayout->addWidget( m_saveButton );
  selectLayout->addWidget( m_addProfileButton );
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
  QLabel *nameValue = new QLabel( "TUXEDO Defaults" );
  detailsLayout->addWidget( nameLabel, row, 0, Qt::AlignTop );
  detailsLayout->addWidget( nameValue, row, 1 );
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
  QLabel *displayHeader = new QLabel( "Display" );
  displayHeader->setStyleSheet( "font-weight: bold; font-size: 14px;" );
  detailsLayout->addWidget( displayHeader, row, 0, 1, 2 );
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
  m_fanProfileCombo = new QComboBox();
  m_fanProfileCombo->addItems( QStringList() << "Quiet" << "Balanced" << "Performance" << "Custom" );
  m_fanProfileCombo->setCurrentIndex( 1 );
  detailsLayout->addWidget( fanProfileLabel, row, 0 );
  detailsLayout->addWidget( m_fanProfileCombo, row, 1 );
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
  QLabel *tdp1Label = new QLabel( "" );  // Empty label for alignment
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
  QLabel *tdp2Label = new QLabel( "" );
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
  QLabel *tdp3Label = new QLabel( "" );
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

  QLabel *maxPerfLabel = new QLabel( "Maximum performance" );
  m_maxPerformanceCheckBox = new QCheckBox();
  m_maxPerformanceCheckBox->setChecked( true );
  detailsLayout->addWidget( maxPerfLabel, row, 0 );
  detailsLayout->addWidget( m_maxPerformanceCheckBox, row, 1, Qt::AlignLeft );
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
  m_gpuPowerSlider->setMaximum( 200 );
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

void MainWindow::setupHardwarePage()
{
  QWidget *hardwareWidget = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout( hardwareWidget );
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

  m_tabs->addTab( hardwareWidget, "Hardware" );
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
    m_activeProfileLabel->setText( m_profileManager->activeProfile() );
    loadProfileDetails( m_profileManager->activeProfile() );
    updateFanTabVisibility();
  } );

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

  // Dashboard page connections

  connect( m_systemMonitor.get(), &SystemMonitor::cpuTempChanged,
           this, &MainWindow::onCpuTempChanged );

  connect( m_systemMonitor.get(), &SystemMonitor::cpuFrequencyChanged,
           this, &MainWindow::onCpuFrequencyChanged );

  connect( m_systemMonitor.get(), &SystemMonitor::cpuPowerChanged,
           this, &MainWindow::onCpuPowerChanged );

  connect( m_systemMonitor.get(), &SystemMonitor::gpuTempChanged,
           this, &MainWindow::onGpuTempChanged );

  connect( m_systemMonitor.get(), &SystemMonitor::gpuFrequencyChanged,
           this, &MainWindow::onGpuFrequencyChanged );

  connect( m_systemMonitor.get(), &SystemMonitor::gpuPowerChanged,
           this, &MainWindow::onGpuPowerDashboardChanged );

  connect( m_systemMonitor.get(), &SystemMonitor::fanSpeedChanged,
           this, &MainWindow::onFanSpeedChanged );

  connect( m_systemMonitor.get(), &SystemMonitor::gpuFanSpeedChanged,
           this, &MainWindow::onGpuFanSpeedChanged );

  connect( m_displayBrightnessSlider, &QSlider::sliderMoved,
           this, &MainWindow::onDisplayBrightnessSliderChanged );

  connect( m_displayBrightnessSlider, &QSlider::valueChanged,
           this, [this]( int value ) {
    if ( m_displayBrightnessValueLabel )
    {
      m_displayBrightnessValueLabel->setText( QString::number( value ) + "%" );
    }
  } );

  connect( m_webcamCheckBox, &QCheckBox::toggled,
           this, &MainWindow::onWebcamToggled );

  connect( m_fnLockCheckBox, &QCheckBox::toggled,
           this, &MainWindow::onFnLockToggled );

  // Apply and Save buttons

  connect( m_applyButton, &QPushButton::clicked,
           this, &MainWindow::onApplyClicked );

  connect( m_saveButton, &QPushButton::clicked,
           this, &MainWindow::onSaveClicked );

  connect( m_addProfileButton, &QPushButton::clicked,
           this, &MainWindow::onAddProfileClicked );

  connect( m_removeProfileButton, &QPushButton::clicked,
           this, &MainWindow::onRemoveProfileClicked );

  // Connect all profile controls to mark changes

  connect( m_descriptionEdit, &QTextEdit::textChanged,
           this, &MainWindow::markChanged );

  connect( m_setBrightnessCheckBox, &QCheckBox::toggled,
           this, &MainWindow::markChanged );

  connect( m_brightnessSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_fanProfileCombo, QOverload< int >::of( &QComboBox::currentIndexChanged ),
           this, &MainWindow::markChanged );

  connect( m_minFanSpeedSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_maxFanSpeedSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_offsetFanSpeedSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_cpuCoresSlider, &QSlider::valueChanged,
           this, [this]() { markChanged(); } );

  connect( m_maxPerformanceCheckBox, &QCheckBox::toggled,
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
}

// Dashboard slots
void MainWindow::onCpuTempChanged()
{

  if ( m_cpuTempLabel )
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
}

void MainWindow::onCpuFrequencyChanged()
{

  if ( m_cpuFrequencyLabel )
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
}

void MainWindow::onCpuPowerChanged()
{

  if ( m_cpuPowerLabel )
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
}

void MainWindow::onGpuTempChanged()
{

  if ( m_gpuTempLabel )
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
}

void MainWindow::onGpuFrequencyChanged()
{

  if ( m_gpuFrequencyLabel )
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
}

void MainWindow::onGpuPowerDashboardChanged()
{

  if ( m_gpuPowerLabel )
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
}

void MainWindow::onFanSpeedChanged()
{
  QString fan = m_systemMonitor->fanSpeed();
  QString display = "---";
  

  if ( fan.endsWith( " RPM" ) )
  {
    QString rpmStr = fan.left( fan.size() - 4 ).trimmed();
    bool ok = false;
    int rpm = rpmStr.toInt( &ok );

    if ( ok && rpm > 0 )
    {
      display = QString::number( rpm );
    }
  }

  if ( m_fanSpeedLabel )
  {
    m_fanSpeedLabel->setText( display );
  }
}

void MainWindow::onGpuFanSpeedChanged()
{
  QString fan = m_systemMonitor->gpuFanSpeed();
  QString display = "---";
  

  if ( fan.endsWith( " RPM" ) )
  {
    QString rpmStr = fan.left( fan.size() - 4 ).trimmed();
    bool ok = false;
    int rpm = rpmStr.toInt( &ok );

    if ( ok && rpm > 0 )
    {
      display = QString::number( rpm );
    }
  }

  if ( m_gpuFanSpeedLabel )
  {
    m_gpuFanSpeedLabel->setText( display );
  }
}

void MainWindow::onDisplayBrightnessSliderChanged( int value )
{
  m_systemMonitor->setDisplayBrightness( value );
  statusBar()->showMessage( "Brightness set to " + QString::number( value ) + "%" );
}

void MainWindow::onWebcamToggled( bool checked )
{
  m_systemMonitor->setWebcamEnabled( checked );
  statusBar()->showMessage( "Webcam " + QString( checked ? "enabled" : "disabled" ) );
}

void MainWindow::onFnLockToggled( bool checked )
{
  m_systemMonitor->setFnLock( checked );
  statusBar()->showMessage( "Fn Lock " + QString( checked ? "enabled" : "disabled" ) );
}

void MainWindow::onTabChanged( int index )
{
  // Enable monitoring only when dashboard tab (index 0) is visible
  bool isDashboard = ( index == 0 );
  qDebug() << "Tab changed to" << index << "- Monitoring active:" << isDashboard;
  m_systemMonitor->setMonitoringActive( isDashboard );
}

// Profile page slots
void MainWindow::onProfileIndexChanged( int index )
{
  if ( index >= 0 )
  {
    QString profileName = m_profileCombo->currentText();
    qDebug() << "Profile selected:" << profileName << "at index" << index;
    m_selectedProfileIndex = index;
    loadProfileDetails( profileName );
    m_applyButton->setEnabled( true );
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

  // If we were in the middle of saving, mark as complete
  if ( m_saveInProgress )
  {
    m_saveInProgress = false;
    m_profileChanged = false;
    statusBar()->showMessage( "Profile saved successfully" );
    updateButtonStates();
  }

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
  m_applyButton->setEnabled( false );
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
  m_fanProfileCombo->blockSignals( true );
  m_minFanSpeedSlider->blockSignals( true );
  m_maxFanSpeedSlider->blockSignals( true );
  m_offsetFanSpeedSlider->blockSignals( true );
  m_cpuCoresSlider->blockSignals( true );
  m_maxPerformanceCheckBox->blockSignals( true );
  m_minFrequencySlider->blockSignals( true );
  m_maxFrequencySlider->blockSignals( true );
  m_odmPowerLimit1Slider->blockSignals( true );
  m_odmPowerLimit2Slider->blockSignals( true );
  m_odmPowerLimit3Slider->blockSignals( true );
  m_gpuPowerSlider->blockSignals( true );
  m_mainsButton->blockSignals( true );
  m_batteryButton->blockSignals( true );

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

  if ( obj.contains( "fan" ) && obj["fan"].isObject() )
  {
    QJsonObject fanObj = obj["fan"].toObject();
    

    if ( fanObj.contains( "fanProfile" ) )
    {
      QString fanProfile = fanObj["fanProfile"].toString( "Balanced" );
      int idx = m_fanProfileCombo->findText( fanProfile );

      if ( idx >= 0 )
        m_fanProfileCombo->setCurrentIndex( idx );
    }

    if ( fanObj.contains( "minimumFanspeed" ) )
      m_minFanSpeedSlider->setValue( fanObj["minimumFanspeed"].toInt( 0 ) );

    if ( fanObj.contains( "maximumFanspeed" ) )
      m_maxFanSpeedSlider->setValue( fanObj["maximumFanspeed"].toInt( 70 ) );

    if ( fanObj.contains( "offsetFanspeed" ) )
      m_offsetFanSpeedSlider->setValue( fanObj["offsetFanspeed"].toInt( 0 ) );
  }

  // Load CPU settings (nested in cpu object)

  if ( obj.contains( "cpu" ) && obj["cpu"].isObject() )
  {
    QJsonObject cpuObj = obj["cpu"].toObject();
    

    if ( cpuObj.contains( "onlineCores" ) )
      m_cpuCoresSlider->setValue( cpuObj["onlineCores"].toInt( 32 ) );

    if ( cpuObj.contains( "useMaxPerfGov" ) )
      m_maxPerformanceCheckBox->setChecked( cpuObj["useMaxPerfGov"].toBool( true ) );

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
      }
    }
  }

  // Unblock signals
  m_brightnessSlider->blockSignals( false );
  m_setBrightnessCheckBox->blockSignals( false );
  m_fanProfileCombo->blockSignals( false );
  m_minFanSpeedSlider->blockSignals( false );
  m_maxFanSpeedSlider->blockSignals( false );
  m_offsetFanSpeedSlider->blockSignals( false );
  m_cpuCoresSlider->blockSignals( false );
  m_maxPerformanceCheckBox->blockSignals( false );
  m_minFrequencySlider->blockSignals( false );
  m_maxFrequencySlider->blockSignals( false );
  m_odmPowerLimit1Slider->blockSignals( false );
  m_odmPowerLimit2Slider->blockSignals( false );
  m_odmPowerLimit3Slider->blockSignals( false );
  m_gpuPowerSlider->blockSignals( false );
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
  
  fprintf( log, "Profile details loaded for: %s\n", profileName.toStdString().c_str() );
  fprintf( log, "Final slider values: ODM1=%d, ODM2=%d, ODM3=%d\n", 
           m_odmPowerLimit1Slider->value(), m_odmPowerLimit2Slider->value(), m_odmPowerLimit3Slider->value() );
  fprintf( log, "Final slider maxima: ODM1_max=%d, ODM2_max=%d, ODM3_max=%d\n",
           m_odmPowerLimit1Slider->maximum(), m_odmPowerLimit2Slider->maximum(), m_odmPowerLimit3Slider->maximum() );
  fflush( log );
  
  // Fan profiles are now separate from TCC profiles, so no need to load fan curves here
  /*
  // Enable/disable and populate fan curve editors depending on whether profile is editable
  const bool isCustom = m_profileManager ? m_profileManager->isCustomProfile( profileName ) : false;

  // Helper function to parse fan points from JSON array, extracting or interpolating points at 5°C intervals
  auto parseFanPoints = []( const QJsonArray &arr ) -> QVector< FanCurveEditorWidget::Point > {
    QVector< FanCurveEditorWidget::Point > pts;
    
    // Collect all points from the array
    QVector< FanCurveEditorWidget::Point > givenPts;
    for ( int i = 0; i < arr.size(); ++i ) {
      QJsonValue v = arr.at( i );
      if ( v.isObject() ) {
        QJsonObject o = v.toObject();
        FanCurveEditorWidget::Point p;
        p.temp = o.value( "temp" ).toDouble( -1 );
        p.duty = o.value( "speed" ).toDouble( 0 );
        if ( p.temp >= 0 ) {
          givenPts.append( p );
        }
      }
    }
    
    // Sort given points by temp
    std::sort( givenPts.begin(), givenPts.end(), []( const FanCurveEditorWidget::Point &a, const FanCurveEditorWidget::Point &b ) {
      return a.temp < b.temp;
    } );
    
    // Generate 17 points at 5°C intervals from 20 to 100
    for ( int i = 0; i < 17; ++i ) {
      double targetTemp = 20.0 + i * 5.0;
      double targetDuty = 0.0;
      
      // Find the interval in givenPts
      if ( givenPts.size() == 0 ) {
        targetDuty = 0.0;
      } else if ( givenPts.size() == 1 ) {
        targetDuty = givenPts[0].duty;
      } else {
        // Find the two points to interpolate between
        FanCurveEditorWidget::Point left = givenPts[0];
        FanCurveEditorWidget::Point right = givenPts[givenPts.size() - 1];
        for ( int j = 0; j < givenPts.size() - 1; ++j ) {
          if ( targetTemp >= givenPts[j].temp && targetTemp <= givenPts[j+1].temp ) {
            left = givenPts[j];
            right = givenPts[j+1];
            break;
          }
        }
        if ( left.temp == right.temp ) {
          targetDuty = left.duty;
        } else {
          double ratio = (targetTemp - left.temp) / (right.temp - left.temp);
          targetDuty = left.duty + ratio * (right.duty - left.duty);
        }
      }
      
      FanCurveEditorWidget::Point p;
      p.temp = targetTemp;
      p.duty = std::clamp( targetDuty, 0.0, 100.0 );
      pts.append( p );
    }
    
    return pts;
  };

  if ( m_cpuFanCurveEditor )
  {
    m_cpuFanCurveEditor->setEnabled( isCustom );

    if ( isCustom && obj.contains( "fan" ) && obj["fan"].isObject() )
    {
      QJsonObject fanObj = obj["fan"].toObject();

      if ( fanObj.contains( "customFanCurve" ) && fanObj["customFanCurve"].isObject() )
      {
        QJsonObject curveObj = fanObj["customFanCurve"].toObject();

        if ( curveObj.contains( "tableCPU" ) && curveObj["tableCPU"].isArray() )
        {
          QJsonArray cpuArray = curveObj["tableCPU"].toArray();
          
          // If customFanCurve is empty, load from fanProfile
          if ( cpuArray.isEmpty() && fanObj.contains( "fanProfile" ) )
          {
            QString fanProfileName = fanObj["fanProfile"].toString();
            QString fanProfileJson = m_profileManager->getFanProfile( fanProfileName );
            QJsonDocument fanDoc = QJsonDocument::fromJson( fanProfileJson.toUtf8() );
            if ( fanDoc.isObject() )
            {
              QJsonObject fanObj = fanDoc.object();
              if ( fanObj.contains( "tableCPU" ) && fanObj["tableCPU"].isArray() )
              {
                cpuArray = fanObj["tableCPU"].toArray();
              }
            }
          }
          
          QVector< FanCurveEditorWidget::Point > pts = parseFanPoints( cpuArray );

          if ( pts.size() > 0 )
            m_cpuFanCurveEditor->setPoints( pts );
        }
      }
    }
  }

  if ( m_gpuFanCurveEditor )
  {
    m_gpuFanCurveEditor->setEnabled( isCustom );

    if ( isCustom && obj.contains( "fan" ) && obj["fan"].isObject() )
    {
      QJsonObject fanObj = obj["fan"].toObject();

      if ( fanObj.contains( "customFanCurve" ) && fanObj["customFanCurve"].isObject() )
      {
        QJsonObject curveObj = fanObj["customFanCurve"].toObject();

        if ( curveObj.contains( "tableGPU" ) && curveObj["tableGPU"].isArray() )
        {
          QJsonArray gpuArray = curveObj["tableGPU"].toArray();
          
          // If customFanCurve is empty, load from fanProfile
          if ( gpuArray.isEmpty() && fanObj.contains( "fanProfile" ) )
          {
            QString fanProfileName = fanObj["fanProfile"].toString();
            QString fanProfileJson = m_profileManager->getFanProfile( fanProfileName );
            QJsonDocument fanDoc = QJsonDocument::fromJson( fanProfileJson.toUtf8() );
            if ( fanDoc.isObject() )
            {
              QJsonObject fanObj = fanDoc.object();
              if ( fanObj.contains( "tableGPU" ) && fanObj["tableGPU"].isArray() )
              {
                gpuArray = fanObj["tableGPU"].toArray();
              }
            }
          }
          
          QVector< FanCurveEditorWidget::Point > pts = parseFanPoints( gpuArray );

          if ( pts.size() > 0 )
            m_gpuFanCurveEditor->setPoints( pts );
        }
      }
    }
  }
  */

  fclose( log );
}

void MainWindow::markChanged()
{
  m_profileChanged = true;
  updateButtonStates();
}

void MainWindow::updateButtonStates()
{
  const QString profileName = m_profileCombo ? m_profileCombo->currentText() : QString();
  const bool isCustom = m_profileManager ? m_profileManager->isCustomProfile( profileName ) : false;
  const bool isConnected = m_tccdClient && m_tccdClient->isConnected();

  // Apply will be implemented later
  m_applyButton->setEnabled( false );
  m_saveButton->setEnabled( m_profileChanged && isCustom );
  
  // Fan profile buttons

  if ( m_applyFanProfilesButton )
  {
    m_applyFanProfilesButton->setEnabled( isCustom && isConnected );
  }

  if ( m_saveFanProfilesButton )
  {
    m_saveFanProfilesButton->setEnabled( isCustom );
  }

  if ( m_revertFanProfilesButton )
  {
    m_revertFanProfilesButton->setEnabled( isCustom && isConnected );
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
  if ( !m_profileManager->isCustomProfile( m_profileCombo->currentText() ) )
  {
    statusBar()->showMessage( "Built-in profiles are read-only" );
    return;
  }

  // Save changes to profile
  QString profileName = m_profileCombo->currentText();
  QString profileId = m_profileManager->getProfileIdByName( profileName );
  
  // Build updated profile JSON
  QJsonObject profileObj;
  profileObj["id"] = profileId;
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
  fanObj["profile"] = m_fanProfileCombo->currentText();
  fanObj["minSpeed"] = m_minFanSpeedSlider->value();
  fanObj["maxSpeed"] = m_maxFanSpeedSlider->value();
  fanObj["offset"] = m_offsetFanSpeedSlider->value();
  
  /*
  // Add fan curves to the profile JSON if custom profile

  if ( isCustom )
  {
    QJsonObject customFanCurveObj;
    
    // CPU fan curve

    if ( m_cpuFanCurveEditor && m_cpuFanCurveEditor->isEnabled() )
    {
      const auto &pts = m_cpuFanCurveEditor->points();

      if ( pts.size() == 17 )
      {
        QJsonArray cpuArr;
        for ( const auto &p : pts )
        {
          QJsonObject o;
          o[ "temp" ] = static_cast< int >( std::lround( p.temp ) );
          o[ "speed" ] = static_cast< int >( std::lround( p.duty ) );
          cpuArr.append( o );
        }
        customFanCurveObj["tableCPU"] = cpuArr;
      }
    }
    
    // GPU fan curve

    if ( m_gpuFanCurveEditor && m_gpuFanCurveEditor->isEnabled() )
    {
      const auto &pts = m_gpuFanCurveEditor->points();

      if ( pts.size() == 17 )
      {
        QJsonArray gpuArr;
        for ( const auto &p : pts )
        {
          QJsonObject o;
          o[ "temp" ] = static_cast< int >( std::lround( p.temp ) );
          o[ "speed" ] = static_cast< int >( std::lround( p.duty ) );
          gpuArr.append( o );
        }
        customFanCurveObj["tableGPU"] = gpuArr;
      }
    }
    

    if ( !customFanCurveObj.isEmpty() )
    {
      fanObj["customFanCurve"] = customFanCurveObj;
    }
  }
  */

  profileObj["fan"] = fanObj;
  
  // CPU settings
  QJsonObject cpuObj;
  cpuObj["cores"] = m_cpuCoresSlider->value();
  cpuObj["maxPerformance"] = m_maxPerformanceCheckBox->isChecked();
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
  
  // Convert to JSON string and save
  QJsonDocument doc( profileObj );
  QString profileJSON = QString::fromUtf8( doc.toJson() );

  m_profileManager->saveProfile( profileJSON );
  
  // Update stateMap based on mains/battery button states
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
  } else {
    QMessageBox::warning(this, "Error", "Failed to create profile.");
  }
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

void MainWindow::onFanProfileChanged(const QString& profileName)
{
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
  
  // Set editors as editable only for Custom profile
  bool isEditable = (profileName == "Custom");
  if (m_cpuFanCurveEditor) {
    m_cpuFanCurveEditor->setEditable(isEditable);
  }
  if (m_gpuFanCurveEditor) {
    m_gpuFanCurveEditor->setEditable(isEditable);
  }
}

void MainWindow::onCpuFanPointsChanged(const QVector<FanCurveEditorWidget::Point>& points)
{
  m_cpuFanPoints.clear();
  for (const auto& p : points) {
    m_cpuFanPoints.append({static_cast<int>(p.temp), static_cast<int>(p.duty)});
  }
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
  if (currentProfile == "Custom") {
    // Already on Custom, no need to copy
    return;
  }
  
  // Get the current profile data
  QString json = m_profileManager->getFanProfile(currentProfile);
  
  // Save it as Custom
  if (m_profileManager->setFanProfile("Custom", json)) {
    // Switch to Custom profile
    m_fanProfileCombo->setCurrentText("Custom");
    statusBar()->showMessage( QString("Copied '%1' profile to Custom").arg(currentProfile) );
  } else {
    QMessageBox::warning(this, "Error", "Failed to copy profile to Custom.");
  }
}

void MainWindow::onApplyFanProfilesClicked()
{
  saveFanPoints();
}

void MainWindow::onRevertFanProfilesClicked()
{
  loadFanPoints();
}

void MainWindow::loadFanPoints()
{
  QString json = m_profileManager->getFanProfile("Custom");
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
  m_profileManager->setFanProfile("Custom", json);
}

} // namespace ucc
