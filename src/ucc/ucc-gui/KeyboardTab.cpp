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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QRadioButton>
#include <QButtonGroup>
#include <QColorDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QInputDialog>

namespace ucc
{

void MainWindow::setupKeyboardBacklightPage()
{
  QWidget *keyboardWidget = new QWidget();
  QVBoxLayout *mainLayout = new QVBoxLayout( keyboardWidget );
  mainLayout->setContentsMargins( 0, 0, 0, 0 );
  mainLayout->setSpacing( 0 );

  // Keyboard profile controls
  QHBoxLayout *profileLayout = new QHBoxLayout();
  QLabel *profileLabel = new QLabel( "Profile:" );
  profileLabel->setStyleSheet( "font-weight: bold;" );
  m_keyboardProfileCombo = new QComboBox();
  
  // Add custom keyboard profiles from settings
  for ( const auto &name : m_profileManager->customKeyboardProfiles() )
    m_keyboardProfileCombo->addItem( name );
  
  m_addKeyboardProfileButton = new QPushButton("Add");
  m_addKeyboardProfileButton->setMaximumWidth(60);
  
  m_copyKeyboardProfileButton = new QPushButton("Copy");
  m_copyKeyboardProfileButton->setMaximumWidth(60);
  m_copyKeyboardProfileButton->setEnabled( false );
  
  m_saveKeyboardProfileButton = new QPushButton("Save");
  m_saveKeyboardProfileButton->setMaximumWidth(60);
  m_saveKeyboardProfileButton->setEnabled( true ); // Always enabled - can always save current state
  
  m_removeKeyboardProfileButton = new QPushButton("Remove");
  m_removeKeyboardProfileButton->setMaximumWidth(70);
  
  profileLayout->addWidget( profileLabel );
  profileLayout->addWidget( m_keyboardProfileCombo, 1 );
  profileLayout->addWidget( m_addKeyboardProfileButton );
  profileLayout->addWidget( m_copyKeyboardProfileButton );
  profileLayout->addWidget( m_saveKeyboardProfileButton );
  profileLayout->addWidget( m_removeKeyboardProfileButton );
  profileLayout->addStretch();
  mainLayout->addLayout( profileLayout );

  // Add a separator line
  QFrame *separator = new QFrame();
  separator->setFrameShape( QFrame::HLine );
  separator->setStyleSheet( "color: #cccccc;" );
  mainLayout->addWidget( separator );

  // Check if keyboard backlight is supported
  if ( auto info = m_UccdClient->getKeyboardBacklightInfo() )
  {
    // Parse the JSON to get capabilities
    if ( QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( *info ).toUtf8() ); doc.isObject() )
    {
      QJsonObject caps = doc.object();
      int zones = caps["zones"].toInt();
      int maxBrightness = caps["maxBrightness"].toInt();
      int maxRed = caps["maxRed"].toInt();
      int maxGreen = caps["maxGreen"].toInt();
      int maxBlue = caps["maxBlue"].toInt();

      if ( zones > 0 )
      {
        QHBoxLayout *brightnessLayout = new QHBoxLayout();
        brightnessLayout->setContentsMargins( 20, 20, 20, 20 );
        brightnessLayout->setSpacing( 15 );

        QLabel *brightnessLabel = new QLabel( "Brightness:" );
        m_keyboardBrightnessSlider = new QSlider( Qt::Horizontal );
        m_keyboardBrightnessSlider->setMinimum( 0 );
        m_keyboardBrightnessSlider->setMaximum( maxBrightness );
        m_keyboardBrightnessSlider->setValue( maxBrightness / 2 );
        m_keyboardBrightnessValueLabel = new QLabel( QString::number( maxBrightness / 2 ) );
        m_keyboardBrightnessValueLabel->setMinimumWidth( 40 );

        brightnessLayout->addWidget( brightnessLabel );
        brightnessLayout->addWidget( m_keyboardBrightnessSlider );
        brightnessLayout->addWidget( m_keyboardBrightnessValueLabel );

        mainLayout->addLayout( brightnessLayout );

        // Color mode selection for multi-zone keyboards
        if ( zones > 1 )
        {
          QHBoxLayout *modeLayout = new QHBoxLayout();
          modeLayout->setContentsMargins( 20, 10, 20, 10 );

          QLabel *modeLabel = new QLabel( "Color Mode:" );
          m_keyboardGlobalColorRadio = new QRadioButton( "Global" );
          m_keyboardPerKeyColorRadio = new QRadioButton( "Per-Key" );
          m_keyboardColorModeGroup = new QButtonGroup( this );
          m_keyboardColorModeGroup->addButton( m_keyboardGlobalColorRadio, 0 );
          m_keyboardColorModeGroup->addButton( m_keyboardPerKeyColorRadio, 1 );
          m_keyboardGlobalColorRadio->setChecked( true );  // Default to global

          modeLayout->addWidget( modeLabel );
          modeLayout->addWidget( m_keyboardGlobalColorRadio );
          modeLayout->addWidget( m_keyboardPerKeyColorRadio );
          modeLayout->addStretch();

          mainLayout->addLayout( modeLayout );
        }

        // Global color controls for RGB keyboards
        if ( maxRed > 0 && maxGreen > 0 && maxBlue > 0 )
        {
          QHBoxLayout *colorLayout = new QHBoxLayout();

          m_keyboardColorLabel = new QLabel( "Color:" );
          m_keyboardColorButton = new QPushButton( "Choose Color" );
          m_keyboardColorButton->setStyleSheet( "background-color: #ffffff;" );

          colorLayout->addWidget( m_keyboardColorLabel );
          colorLayout->addWidget( m_keyboardColorButton );
          colorLayout->addStretch();

          mainLayout->addLayout( colorLayout );

          // Hide if per-key is selected (for multi-zone)
          if ( zones > 1 and m_keyboardPerKeyColorRadio and m_keyboardPerKeyColorRadio->isChecked() )
          {
            m_keyboardColorLabel->setVisible( false );
            m_keyboardColorButton->setVisible( false );
          }
        }

        // Keyboard visualizer
        if ( zones > 1 )
        {
          m_keyboardVisualizerLabel = new QLabel( "Per-Key Control:" );
          m_keyboardVisualizerLabel->setStyleSheet( "font-weight: bold; margin-top: 10px;" );
          mainLayout->addWidget( m_keyboardVisualizerLabel );

          m_keyboardVisualizer = new KeyboardVisualizerWidget( zones, keyboardWidget );
          mainLayout->addWidget( m_keyboardVisualizer );

          // Connect visualizer signals
          connect( m_keyboardVisualizer, &KeyboardVisualizerWidget::colorsChanged,
                   this, &MainWindow::onKeyboardVisualizerColorsChanged );

          // Hide if global is selected
          if ( m_keyboardGlobalColorRadio and m_keyboardGlobalColorRadio->isChecked() )
          {
            m_keyboardVisualizerLabel->setVisible( false );
            m_keyboardVisualizer->setVisible( false );
          }
        }

        // Zone info
        QLabel *infoLabel = new QLabel( QString( "Detected %1 zone(s)" ).arg( zones ) );
        mainLayout->addWidget( infoLabel );
      }
      else
      {
        QLabel *noSupportLabel = new QLabel( "Keyboard backlight not supported on this device." );
        mainLayout->addWidget( noSupportLabel );
      }
    }
  }
  else
  {
    QLabel *noSupportLabel = new QLabel( "Keyboard backlight not available." );
    mainLayout->addWidget( noSupportLabel );
  }

  mainLayout->addStretch();
  m_tabs->addTab( keyboardWidget, "Keyboard" );
}

void MainWindow::reloadKeyboardProfiles()
{
  if ( m_keyboardProfileCombo )
  {
    m_keyboardProfileCombo->clear();
    // Add "Default" profile
    m_keyboardProfileCombo->addItem( "Default" );
    // Add custom keyboard profiles from settings
    for ( const auto &name : m_profileManager->customKeyboardProfiles() )
      m_keyboardProfileCombo->addItem( name );
  }

  // Update button states
  updateKeyboardProfileButtonStates();
}

void MainWindow::updateKeyboardProfileButtonStates()
{
  if ( not m_keyboardProfileCombo or not m_copyKeyboardProfileButton or not m_removeKeyboardProfileButton )
    return;

  QString current = m_keyboardProfileCombo->currentText();
  bool isCustom = m_profileManager->customKeyboardProfiles().contains( current );

  m_copyKeyboardProfileButton->setEnabled( isCustom );
  m_removeKeyboardProfileButton->setEnabled( isCustom );
}

void MainWindow::onKeyboardBrightnessChanged( int value )
{
  if ( m_keyboardBrightnessValueLabel )
    m_keyboardBrightnessValueLabel->setText( QString::number( value ) );

  // Update visualizer if it exists
  if ( m_keyboardVisualizer )
    m_keyboardVisualizer->setGlobalBrightness( value );
  else
  {
    // Fallback for single zone keyboards
    QJsonArray statesArray;
    QJsonObject state;
    state["mode"] = 0; // Static
    state["brightness"] = value;
    state["red"] = 255;
    state["green"] = 255;
    state["blue"] = 255;
    statesArray.append( state );

    QJsonDocument doc( statesArray );
    QString json = doc.toJson( QJsonDocument::Compact );

    if ( not m_UccdClient->setKeyboardBacklight( json.toStdString() ) )
    {
      statusBar()->showMessage( "Failed to set keyboard backlight", 3000 );
    }
  }
}

void MainWindow::onKeyboardColorClicked()
{
  // Open color dialog
  QColor color = QColorDialog::getColor( Qt::white, this, "Choose Keyboard Color" );
  if ( color.isValid() )
  {
    QString style = QString( "background-color: %1;" ).arg( color.name() );
    m_keyboardColorButton->setStyleSheet( style );

    // Update visualizer if it exists
    if ( m_keyboardVisualizer )
      m_keyboardVisualizer->setGlobalColor( color );
    else
    {
      // Fallback for single zone keyboards
      int brightness = m_keyboardBrightnessSlider ? m_keyboardBrightnessSlider->value() : 128;

      QJsonArray statesArray;
      QJsonObject state;
      state["mode"] = 0; // Static
      state["brightness"] = brightness;
      state["red"] = color.red();
      state["green"] = color.green();
      state["blue"] = color.blue();
      statesArray.append( state );

      QJsonDocument doc( statesArray );
      QString json = doc.toJson( QJsonDocument::Compact );

      if ( not m_UccdClient->setKeyboardBacklight( json.toStdString() ) )
      {
        statusBar()->showMessage( "Failed to set keyboard backlight", 3000 );
      }
    }
  }
}

void MainWindow::onKeyboardVisualizerColorsChanged()
{
  if ( not m_keyboardVisualizer )
    return;

  // Get the color data from the visualizer
  QJsonArray statesArray = m_keyboardVisualizer->getJSONState();
  if ( statesArray.empty() )
    return;

  QJsonDocument doc( statesArray );
  QString json = doc.toJson( QJsonDocument::Compact );

  if ( not m_UccdClient->setKeyboardBacklight( json.toStdString() ) )
  {
    statusBar()->showMessage( "Failed to set keyboard backlight", 3000 );
  }
}

void MainWindow::onKeyboardProfileChanged(const QString& profileName)
{
  if ( profileName.isEmpty() or not m_keyboardProfileCombo )
    return;

  // Get the keyboard profile data
  QString json = m_profileManager->getKeyboardProfile( profileName );
  if ( json.isEmpty() or json == "{}" )
  {
    qDebug() << "No keyboard profile data for" << profileName;
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson( json.toUtf8() );
  if ( doc.isArray() )
  {
    QJsonArray statesArray = doc.array();
    
    // Apply to keyboard visualizer if available
    if ( m_keyboardVisualizer )
    {
      m_keyboardVisualizer->updateFromJSON( statesArray );
    }
    else
    {
      // Apply directly to hardware
      if ( not m_UccdClient->setKeyboardBacklight( json.toStdString() ) )
      {
        statusBar()->showMessage( "Failed to load keyboard profile", 3000 );
      }
    }

    // Update brightness slider if present
    if ( not statesArray.isEmpty() and statesArray[ 0 ].isObject() )
    {
      QJsonObject firstState = statesArray[ 0 ].toObject();
      int brightness = firstState["brightness"].toInt(128);
      if ( m_keyboardBrightnessSlider )
        m_keyboardBrightnessSlider->setValue( brightness );
    }
  }

  // Update button states
  updateKeyboardProfileButtonStates();
}

void MainWindow::onKeyboardColorModeChanged( int id )
{
  if ( id == 0 )  // Global
  {
    if ( m_keyboardColorLabel )
      m_keyboardColorLabel->setVisible( true );
    if ( m_keyboardColorButton )
      m_keyboardColorButton->setVisible( true );
    if ( m_keyboardVisualizerLabel )
      m_keyboardVisualizerLabel->setVisible( false );
    if ( m_keyboardVisualizer )
      m_keyboardVisualizer->setVisible( false );
  }
  else if ( id == 1 )  // Per-Key
  {
    if ( m_keyboardColorLabel )
      m_keyboardColorLabel->setVisible( false );
    if ( m_keyboardColorButton )
      m_keyboardColorButton->setVisible( false );
    if ( m_keyboardVisualizerLabel )
      m_keyboardVisualizerLabel->setVisible( true );
    if ( m_keyboardVisualizer )
      m_keyboardVisualizer->setVisible( true );
  }
}

void MainWindow::onAddKeyboardProfileClicked()
{
  bool ok;
  QString name = QInputDialog::getText( this, "Add Keyboard Profile",
                                        "Enter profile name:", QLineEdit::Normal, "", &ok );
  if ( ok and not name.isEmpty() )
  {
    if ( m_profileManager->setKeyboardProfile( name, "{}" ) )  // Add empty profile
    {
      m_keyboardProfileCombo->addItem( name );
      m_keyboardProfileCombo->setCurrentText( name );
      statusBar()->showMessage( QString("Keyboard profile '%1' added").arg(name) );
      updateKeyboardProfileButtonStates();
    }
    else
    {
      QMessageBox::warning( this, "Add Failed", "Failed to add keyboard profile." );
    }
  }
}

void MainWindow::onCopyKeyboardProfileClicked()
{
  QString current = m_keyboardProfileCombo->currentText();
  if ( current.isEmpty() )
    return;

  bool ok;
  QString name = QInputDialog::getText( this, "Copy Keyboard Profile",
                                        "Enter new profile name:", QLineEdit::Normal, current + " Copy", &ok );
  if ( ok and not name.isEmpty() )
  {
    // Get the current profile data and save it with the new name
    QString json = m_profileManager->getKeyboardProfile( current );
    if ( not json.isEmpty() and m_profileManager->setKeyboardProfile( name, json ) )
    {
      m_keyboardProfileCombo->addItem( name );
      m_keyboardProfileCombo->setCurrentText( name );
      statusBar()->showMessage( QString("Keyboard profile '%1' copied to '%2'").arg(current, name) );
      updateKeyboardProfileButtonStates();
    }
    else
    {
      QMessageBox::warning( this, "Copy Failed", "Failed to copy keyboard profile." );
    }
  }
}

void MainWindow::onSaveKeyboardProfileClicked()
{
  QString current = m_keyboardProfileCombo->currentText();
  if ( current.isEmpty() )
    return;

  // Get current keyboard state
  QString json;
  if ( m_keyboardVisualizer )
  {
    // Get from visualizer
    QJsonArray statesArray = m_keyboardVisualizer->getJSONState();
    QJsonDocument doc( statesArray );
    json = doc.toJson( QJsonDocument::Compact );
  }
  else
  {
    // Get from hardware
    if ( auto states = m_UccdClient->getKeyboardBacklightStates() )
    {
      json = QString::fromStdString( *states );
    }
  }

  if ( json.isEmpty() )
  {
    QMessageBox::warning( this, "Save Failed", "Unable to get current keyboard state." );
    return;
  }

  if ( m_profileManager->setKeyboardProfile( current, json ) )
  {
    statusBar()->showMessage( QString("Keyboard profile '%1' saved").arg(current) );
  }
  else
  {
    QMessageBox::warning( this, "Save Failed", "Failed to save keyboard profile." );
  }
}

void MainWindow::onRemoveKeyboardProfileClicked()
{
  QString currentProfile = m_keyboardProfileCombo->currentText();
  
  // Confirm deletion
  QMessageBox::StandardButton reply = QMessageBox::question(
    this, "Remove Keyboard Profile",
    QString("Are you sure you want to remove the keyboard profile '%1'?").arg(currentProfile),
    QMessageBox::Yes | QMessageBox::No
  );
  
  if (reply == QMessageBox::Yes) {
    // Remove from persistent storage and UI
    if ( m_profileManager->deleteKeyboardProfile( currentProfile ) ) {
      int idx = m_keyboardProfileCombo->currentIndex();
      if ( idx >= 0 ) m_keyboardProfileCombo->removeItem( idx );
      statusBar()->showMessage( QString("Keyboard profile '%1' removed").arg(currentProfile) );
      
      // Update button states
      updateKeyboardProfileButtonStates();
    } else {
      QMessageBox::warning(this, "Remove Failed", "Failed to remove custom keyboard profile.");
    }
  }
}

} // namespace ucc