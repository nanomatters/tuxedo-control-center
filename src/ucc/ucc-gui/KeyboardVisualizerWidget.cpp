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

#include "KeyboardVisualizerWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QColorDialog>
#include <QApplication>
#include <QStyle>
#include <algorithm>

namespace ucc
{

KeyboardVisualizerWidget::KeyboardVisualizerWidget( int zones, QWidget *parent )
  : QWidget( parent )
  , m_zones( zones )
{
  setupKeyboardLayout();

  // Initialize color dialog
  m_colorDialog = new QColorDialog( this );
  m_colorDialog->setOption( QColorDialog::ShowAlphaChannel, false );
  m_colorDialog->setOption( QColorDialog::DontUseNativeDialog, false );

  connect( m_colorDialog, &QColorDialog::colorSelected, this, &KeyboardVisualizerWidget::onColorChanged );

  // Record known mappings
  recordZoneMapping( 0, "Left Ctrl" );
  recordZoneMapping( 1, "Unused/Unknown" );
  recordZoneMapping( 2, "Fn" );
  recordZoneMapping( 3, "Left Windows" );
  recordZoneMapping( 4, "Left Alt" );
  recordZoneMapping( 5, "Unused/Unknown" );
  recordZoneMapping( 6, "Unused/Unknown" );
  recordZoneMapping( 7, "Space" );
  recordZoneMapping( 8, "Unused/Unknown" );
  recordZoneMapping( 9, "Unused/Unknown" );
  recordZoneMapping( 10, "Right Alt" );
  recordZoneMapping( 11, "Unused/Unknown" );
  recordZoneMapping( 12, "Right Ctrl" );
  recordZoneMapping( 13, "Left Arrow" );
  recordZoneMapping( 14, "Up Arrow" );
  recordZoneMapping( 15, "Right Arrow" );
  recordZoneMapping( 16, "Numpad 0" );
  recordZoneMapping( 17, "Numpad ," );
  recordZoneMapping( 18, "Down Arrow" );
  recordZoneMapping( 19, "Unused/Unknown" );
  recordZoneMapping( 20, "Unused/Unknown" );
  recordZoneMapping( 21, "Unused/Unknown" );
  recordZoneMapping( 22, "Left Shift" );
  recordZoneMapping( 23, "<" );
  recordZoneMapping( 24, "y" );
  recordZoneMapping( 25, "x" );
  recordZoneMapping( 26, "c" );
  recordZoneMapping( 27, "v" );
  recordZoneMapping( 28, "b" );
  recordZoneMapping( 29, "n" );
  recordZoneMapping( 30, "m" );
  recordZoneMapping( 31, "," );
  recordZoneMapping( 32, "." );
  recordZoneMapping( 33, "-" );
  recordZoneMapping( 34, "Unused/Unknown" );
  recordZoneMapping( 35, "Right Shift" );
  recordZoneMapping( 36, "Numpad 1" );
  recordZoneMapping( 37, "Numpad 2" );
  recordZoneMapping( 38, "Numpad 3" );
  recordZoneMapping( 39, "Numpad Enter" );
  recordZoneMapping( 40, "Unused/Unknown" );
  recordZoneMapping( 41, "Unused/Unknown" );
  recordZoneMapping( 42, "Caps Lock" );
  recordZoneMapping( 43, "Unused/Unknown" );
  recordZoneMapping( 44, "a" );
  recordZoneMapping( 45, "s" );
  recordZoneMapping( 46, "d" );
  recordZoneMapping( 47, "f" );
  recordZoneMapping( 48, "g" );
  recordZoneMapping( 49, "h" );
  recordZoneMapping( 50, "j" );
  recordZoneMapping( 51, "k" );
  recordZoneMapping( 52, "l" );
  recordZoneMapping( 53, "ö" );
  recordZoneMapping( 54, "ä" );
  recordZoneMapping( 55, "#" );
  recordZoneMapping( 56, "Unused/Unknown" );
  recordZoneMapping( 57, "Numpad 4" );
  recordZoneMapping( 58, "Numpad 5" );
  recordZoneMapping( 59, "Numpad 6" );
  recordZoneMapping( 60, "Unused/Unknown" );
  recordZoneMapping( 61, "Unused/Unknown" );
  recordZoneMapping( 62, "Unused/Unknown" );
  recordZoneMapping( 63, "Tab" );
  recordZoneMapping( 64, "Unused/Unknown" );
  recordZoneMapping( 65, "q" );
  recordZoneMapping( 66, "w" );
  recordZoneMapping( 67, "e" );
  recordZoneMapping( 68, "r" );
  recordZoneMapping( 69, "t" );
  recordZoneMapping( 70, "z" );
  recordZoneMapping( 71, "u" );
  recordZoneMapping( 72, "i" );
  recordZoneMapping( 73, "o" );
  recordZoneMapping( 74, "p" );
  recordZoneMapping( 75, "ü" );
  recordZoneMapping( 76, "+" );
  recordZoneMapping( 77, "Enter" );
  recordZoneMapping( 78, "Numpad 7" );
  recordZoneMapping( 79, "Numpad 8" );
  recordZoneMapping( 80, "Numpad 9" );
  recordZoneMapping( 81, "Numpad +" );
  recordZoneMapping( 82, "Unused/Unknown" );
  recordZoneMapping( 83, "Unused/Unknown" );
  recordZoneMapping( 84, "^" );
  recordZoneMapping( 85, "1" );
  recordZoneMapping( 86, "2" );
  recordZoneMapping( 87, "3" );
  recordZoneMapping( 88, "4" );
  recordZoneMapping( 89, "5" );
  recordZoneMapping( 90, "6" );
  recordZoneMapping( 91, "7" );
  recordZoneMapping( 92, "8" );
  recordZoneMapping( 93, "9" );
  recordZoneMapping( 94, "0" );
  recordZoneMapping( 95, "ß" );
  recordZoneMapping( 96, "'" );
  recordZoneMapping( 97, "Unused/Unknown" );
  recordZoneMapping( 98, "Backspace" );
  recordZoneMapping( 99, "Num Lock" );
  recordZoneMapping( 100, "Numpad /" );
  recordZoneMapping( 101, "Numpad *" );
  recordZoneMapping( 102, "Numpad -" );
  recordZoneMapping( 103, "Unused/Unknown" );
  recordZoneMapping( 104, "Unused/Unknown" );
  recordZoneMapping( 105, "Esc" );
  recordZoneMapping( 106, "F1" );
  recordZoneMapping( 107, "F2" );
  recordZoneMapping( 108, "F3" );
  recordZoneMapping( 109, "F4" );
  recordZoneMapping( 110, "F5" );
  recordZoneMapping( 111, "F6" );
  recordZoneMapping( 112, "F7" );
  recordZoneMapping( 113, "F8" );
  recordZoneMapping( 114, "F9" );
  recordZoneMapping( 115, "F10" );
  recordZoneMapping( 116, "F11" );
  recordZoneMapping( 117, "F12" );
  recordZoneMapping( 118, "Print Screen" );
  recordZoneMapping( 119, "Insert" );
  recordZoneMapping( 120, "Delete" );
  recordZoneMapping( 121, "Home" );
  recordZoneMapping( 122, "End" );
  recordZoneMapping( 123, "Page Up" );
  recordZoneMapping( 124, "Page Down" );
  recordZoneMapping( 125, "Unused/Unknown" );
}

void KeyboardVisualizerWidget::setupKeyboardLayout()
{
  QVBoxLayout *mainLayout = new QVBoxLayout( this );
  mainLayout->setContentsMargins( 0, 0, 0, 0 );

  // Create scroll area for large keyboards
  m_scrollArea = new QScrollArea( this );
  m_scrollArea->setWidgetResizable( true );
  m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
  m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );

  m_keyboardWidget = new QWidget();
  m_layout = new QGridLayout( m_keyboardWidget );
  m_layout->setSpacing( 1 );
  m_layout->setContentsMargins( 5, 5, 5, 5 );

  // For 126 zones, show zone numbers to identify correct hardware mapping
  // TEMP: Display zone numbers instead of key labels to map hardware zones to physical keys
  int currentZone = 0;

  // Row 0: Esc + F1-F12 (13 keys)
  int zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 0, 0 );
  for ( int i = 1; i <= 12; ++i )
  {
    zoneId = currentZone++;
    createKeyButton( zoneId, QString::number(zoneId), 0, i );
  }

  // Row 1: Number row `1234567890-= + Backspace (14 keys)
  QString numberRow = "`1234567890-=";
  for ( int i = 0; i < numberRow.length(); ++i )
  {
    zoneId = currentZone++;
    createKeyButton( zoneId, QString::number(zoneId), 1, i );
  }
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 1, 13, 2, 1 );  // Backspace (wider)

  // Row 2: Tab + QWERTYUIOP[]\ (14 keys)
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 2, 0, 2, 1 );  // Tab (wider)
  QString qwertyRow = "qwertyuiop[]";
  for ( int i = 0; i < qwertyRow.length(); ++i )
  {
    zoneId = currentZone++;
    createKeyButton( zoneId, QString::number(zoneId), 2, i + 2 );
  }
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 2, 14 );  // Backslash

  // Row 3: Caps + ASDFGHJKL;' + Enter (14 keys)
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 3, 0, 2, 1 );  // Caps lock (wider)
  QString asdfRow = "asdfghjkl;'";
  for ( int i = 0; i < asdfRow.length(); ++i )
  {
    zoneId = currentZone++;
    createKeyButton( zoneId, QString::number(zoneId), 3, i + 2 );
  }
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 3, 14, 1, 2 );  // Enter (taller)

  // Row 4: Left Shift + ZXCVBNM,./ + Right Shift (13 keys)
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 4, 0, 2, 1 );  // Left shift (wider)
  QString zxcvRow = "zxcvbnm,./";
  for ( int i = 0; i < zxcvRow.length(); ++i )
  {
    zoneId = currentZone++;
    createKeyButton( zoneId, QString::number(zoneId), 4, i + 2 );
  }
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 4, 12, 2, 1 );  // Right shift (wider)

  // Row 5: Ctrl + Fn + Win + Alt + Space + Alt + Win + Menu + Ctrl (9 keys)
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 5, 0 );
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 5, 1 );
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 5, 2 );  // Windows key
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 5, 3 );
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 5, 4, 5, 1 );  // Space bar (much wider)
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 5, 9 );
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 5, 10 );
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 5, 11 );
  zoneId = currentZone++;
  createKeyButton( zoneId, QString::number(zoneId), 5, 12 );

  // Fill remaining zones with placeholder buttons (if any)
  while ( currentZone < 126 )
  {
    zoneId = currentZone++;
    createKeyButton( zoneId, QString::number(zoneId), 6 + ( currentZone - 70 ) / 15, ( currentZone - 70 ) % 15 );
  }

  m_scrollArea->setWidget( m_keyboardWidget );
  mainLayout->addWidget( m_scrollArea );

  // Add mapping controls
  QHBoxLayout *mappingLayout = new QHBoxLayout();
  mappingLayout->setContentsMargins( 5, 5, 5, 5 );

  m_startMappingButton = new QPushButton( "Start Zone Mapping" );
  m_startMappingButton->setToolTip( "Set all zones to maximum brightness for mapping" );
  connect( m_startMappingButton, &QPushButton::clicked, this, &KeyboardVisualizerWidget::startZoneMapping );

  m_testNextZoneButton = new QPushButton( "Test Next Zone" );
  m_testNextZoneButton->setToolTip( "Set the next zone to blue color to identify which key it corresponds to" );
  m_testNextZoneButton->setEnabled( false );
  connect( m_testNextZoneButton, &QPushButton::clicked, this, &KeyboardVisualizerWidget::onTestNextZone );

  m_mappingStatusLabel = new QLabel( "Click 'Start Zone Mapping' to begin mapping zones to keys" );
  m_mappingStatusLabel->setStyleSheet( "font-size: 11px; color: #666;" );

  mappingLayout->addWidget( m_startMappingButton );
  mappingLayout->addWidget( m_testNextZoneButton );
  mappingLayout->addWidget( m_mappingStatusLabel );
  mappingLayout->addStretch();

  mainLayout->addLayout( mappingLayout );

  // Instructions label
  QLabel *instructions = new QLabel( "Click on keys to select and change their color. Use global controls for all keys." );
  instructions->setStyleSheet( "font-size: 11px; color: #888; margin-top: 5px;" );
  instructions->setWordWrap( true );
  mainLayout->addWidget( instructions );
}

void ucc::KeyboardVisualizerWidget::createKeyButton( int zoneId, const QString &label, int row, int col, int width, int height )
{
  QPushButton *button = new QPushButton( label );
  button->setFixedSize( 25 * width, 25 * height );  // Smaller keys: 25x25 instead of 40x40
  button->setProperty( "zoneId", zoneId );

  // Default white color with full brightness
  updateKeyAppearance( button, Qt::white, 255 );

  // Store key info
  KeyboardKey key;
  key.zoneId = zoneId;
  key.label = label;
  key.color = Qt::white;
  key.brightness = 255;
  m_keys.push_back( key );

  connect( button, &QPushButton::clicked, this, &KeyboardVisualizerWidget::onKeyClicked );

  m_layout->addWidget( button, row, col, height, width );
}

void ucc::KeyboardVisualizerWidget::onKeyClicked()
{
  QPushButton *button = qobject_cast< QPushButton * >( sender() );
  if ( !button )
    return;

  int zoneId = button->property( "zoneId" ).toInt();

  // Clear previous selection
  if ( m_selectedButton )
  {
    QString style = m_selectedButton->styleSheet();
    style.replace( "border: 3px solid #ff0000;", "border: 1px solid #666;" );
    m_selectedButton->setStyleSheet( style );
  }

  // Select new key
  m_selectedZoneId = zoneId;
  m_selectedButton = button;

  // Highlight selected key
  QString currentStyle = button->styleSheet();
  if ( !currentStyle.contains( "border:" ) )
  {
    currentStyle += "border: 3px solid #ff0000;";
  }
  else
  {
    currentStyle.replace( QRegularExpression( "border: \\d+px solid #[0-9a-fA-F]+;" ), "border: 3px solid #ff0000;" );
  }
  button->setStyleSheet( currentStyle );

  emit keySelected( zoneId );

  // Show color dialog for selected key
  if ( zoneId >= 0 && zoneId < static_cast< int >( m_keys.size() ) )
  {
    m_colorDialog->setCurrentColor( m_keys[zoneId].color );
    m_colorDialog->show();
  }
}

void ucc::KeyboardVisualizerWidget::onColorChanged( const QColor &color )
{
  if ( m_selectedZoneId >= 0 && m_selectedZoneId < static_cast< int >( m_keys.size() ) )
  {
    m_keys[m_selectedZoneId].color = color;
    updateKeyAppearance( m_selectedButton, color, m_keys[m_selectedZoneId].brightness );
    emit colorsChanged();
  }
}

void ucc::KeyboardVisualizerWidget::updateKeyAppearance( QPushButton *button, const QColor &color, int brightness )
{
  QColor adjustedColor = applyBrightness( color, brightness );
  QString style = QString( "background-color: %1; color: %2; border: 1px solid #666; border-radius: 3px;" )
                  .arg( adjustedColor.name() )
                  .arg( adjustedColor.lightness() > 128 ? "#000000" : "#ffffff" );
  button->setStyleSheet( style );
}

QColor ucc::KeyboardVisualizerWidget::applyBrightness( const QColor &color, int brightness ) const
{
  // Scale brightness from 0-255 to 0.0-1.0
  double factor = brightness / 255.0;

  int r = qMin( 255, static_cast< int >( color.red() * factor ) );
  int g = qMin( 255, static_cast< int >( color.green() * factor ) );
  int b = qMin( 255, static_cast< int >( color.blue() * factor ) );

  return QColor( r, g, b );
}

void ucc::KeyboardVisualizerWidget::loadCurrentStates( const std::string &statesJSON )
{
  QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( statesJSON ).toUtf8() );
  if ( doc.isArray() )
  {
    updateFromJSON( doc.array() );
  }
}

void ucc::KeyboardVisualizerWidget::updateFromJSON( const QJsonArray &states )
{
  for ( int i = 0; i < states.size() && i < static_cast< int >( m_keys.size() ); ++i )
  {
    QJsonObject state = states[i].toObject();
    QColor color( state["red"].toInt(), state["green"].toInt(), state["blue"].toInt() );
    int brightness = state["brightness"].toInt();

    m_keys[i].color = color;
    m_keys[i].brightness = brightness;

    // Find the button for this zone
    for ( int j = 0; j < m_layout->count(); ++j )
    {
      QLayoutItem *item = m_layout->itemAt( j );
      if ( item && item->widget() )
      {
        QPushButton *button = qobject_cast< QPushButton * >( item->widget() );
        if ( button && button->property( "zoneId" ).toInt() == i )
        {
          updateKeyAppearance( button, color, brightness );
          break;
        }
      }
    }
  }
}

QJsonArray ucc::KeyboardVisualizerWidget::getJSONState() const
{
  QJsonArray states;
  for ( const auto &key : m_keys )
  {
    QJsonObject state;
    state["mode"] = 0;  // Static mode
    state["brightness"] = key.brightness;
    state["red"] = key.color.red();
    state["green"] = key.color.green();
    state["blue"] = key.color.blue();
    states.append( state );
  }
  return states;
}

void ucc::KeyboardVisualizerWidget::setGlobalBrightness( int brightness )
{
  for ( auto &key : m_keys )
  {
    key.brightness = brightness;
  }

  // Update all button appearances
  for ( int i = 0; i < m_layout->count(); ++i )
  {
    QLayoutItem *item = m_layout->itemAt( i );
    if ( item && item->widget() )
    {
      QPushButton *button = qobject_cast< QPushButton * >( item->widget() );
      if ( button )
      {
        int zoneId = button->property( "zoneId" ).toInt();
        if ( zoneId >= 0 && zoneId < static_cast< int >( m_keys.size() ) )
        {
          updateKeyAppearance( button, m_keys[zoneId].color, brightness );
        }
      }
    }
  }

  emit colorsChanged();
}

void ucc::KeyboardVisualizerWidget::setGlobalColor( const QColor &color )
{
  for ( auto &key : m_keys )
  {
    key.color = color;
  }

  // Update all button appearances
  for ( int i = 0; i < m_layout->count(); ++i )
  {
    QLayoutItem *item = m_layout->itemAt( i );
    if ( item && item->widget() )
    {
      QPushButton *button = qobject_cast< QPushButton * >( item->widget() );
      if ( button )
      {
        int zoneId = button->property( "zoneId" ).toInt();
        if ( zoneId >= 0 && zoneId < static_cast< int >( m_keys.size() ) )
        {
          updateKeyAppearance( button, color, m_keys[zoneId].brightness );
        }
      }
    }
  }

  emit colorsChanged();
}

void KeyboardVisualizerWidget::startZoneMapping()
{
  m_mappingMode = true;
  m_currentTestZone = 0;

  // Set all zones to maximum brightness (255) for mapping
  setGlobalBrightness( 255 );

  // Enable the test button
  m_testNextZoneButton->setEnabled( true );
  m_mappingStatusLabel->setText( "Mapping mode active. Click 'Test Next Zone' to set zones to blue one by one." );

  emit colorsChanged();
}

void KeyboardVisualizerWidget::testZone( int zoneId )
{
  if ( zoneId < 0 || zoneId >= m_zones )
    return;

  // Set all zones to maximum brightness with white color
  for ( auto &key : m_keys )
  {
    key.brightness = 255;
    key.color = Qt::white;
  }

  // Set the specific zone to maximum brightness with blue color
  if ( zoneId < static_cast< int >( m_keys.size() ) )
  {
    m_keys[zoneId].brightness = 255;
    m_keys[zoneId].color = Qt::blue;
  }

  // Update all button appearances
  for ( int i = 0; i < m_layout->count(); ++i )
  {
    QLayoutItem *item = m_layout->itemAt( i );
    if ( item && item->widget() )
    {
      QPushButton *button = qobject_cast< QPushButton * >( item->widget() );
      if ( button )
      {
        int buttonZoneId = button->property( "zoneId" ).toInt();
        if ( buttonZoneId >= 0 && buttonZoneId < static_cast< int >( m_keys.size() ) )
        {
          updateKeyAppearance( button, m_keys[buttonZoneId].color, m_keys[buttonZoneId].brightness );
        }
      }
    }
  }

  emit colorsChanged();
}

int KeyboardVisualizerWidget::getNextTestZone()
{
  int nextZone = m_currentTestZone;
  m_currentTestZone = ( m_currentTestZone + 1 ) % m_zones;
  return nextZone;
}

void KeyboardVisualizerWidget::onTestNextZone()
{
  if ( !m_mappingMode )
    return;

  int zoneToTest = getNextTestZone();
  testZone( zoneToTest );

  m_mappingStatusLabel->setText( QString( "Testing zone %1. Which key turned blue? Tell me and I'll record the mapping." ).arg( zoneToTest ) );
}

void KeyboardVisualizerWidget::recordZoneMapping( int zoneId, const QString &keyName )
{
  m_zoneMappings[zoneId] = keyName;
  m_mappingStatusLabel->setText( QString( "Recorded: Zone %1 = %2. %3 mappings recorded so far." )
                                .arg( zoneId )
                                .arg( keyName )
                                .arg( m_zoneMappings.size() ) );
}

QString KeyboardVisualizerWidget::getZoneMappings() const
{
  QString result = "Zone Mappings:\n";
  for ( const auto &mapping : m_zoneMappings )
  {
    result += QString( "Zone %1: %2\n" ).arg( mapping.first ).arg( mapping.second );
  }
  return result;
}

} // namespace ucc