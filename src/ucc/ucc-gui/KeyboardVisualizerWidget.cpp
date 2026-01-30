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
  // Record known mappings FIRST
  recordZoneMapping( 0, "Ctrl" );
  recordZoneMapping( 1, "Unused/Unknown" );
  recordZoneMapping( 2, "Fn" );
  recordZoneMapping( 3, "⊞" );
  recordZoneMapping( 4, "Alt" );
  recordZoneMapping( 5, "Unused/Unknown" );
  recordZoneMapping( 6, "Unused/Unknown" );
  recordZoneMapping( 7, "Space" );
  recordZoneMapping( 8, "Unused/Unknown" );
  recordZoneMapping( 9, "Unused/Unknown" );
  recordZoneMapping( 10, "Alt" );
  recordZoneMapping( 11, "Unused/Unknown" );
  recordZoneMapping( 12, "Ctrl" );
  recordZoneMapping( 13, "←" );
  recordZoneMapping( 14, "↑" );
  recordZoneMapping( 15, "→" );
  recordZoneMapping( 16, "0" );
  recordZoneMapping( 17, "," );
  recordZoneMapping( 18, "↓" );
  recordZoneMapping( 19, "Unused/Unknown" );
  recordZoneMapping( 20, "Unused/Unknown" );
  recordZoneMapping( 21, "Unused/Unknown" );
  recordZoneMapping( 22, "⇧" );
  recordZoneMapping( 23, "<" );
  recordZoneMapping( 24, "Y" );
  recordZoneMapping( 25, "X" );
  recordZoneMapping( 26, "C" );
  recordZoneMapping( 27, "V" );
  recordZoneMapping( 28, "B" );
  recordZoneMapping( 29, "N" );
  recordZoneMapping( 30, "M" );
  recordZoneMapping( 31, "," );
  recordZoneMapping( 32, "." );
  recordZoneMapping( 33, "-" );
  recordZoneMapping( 34, "Unused/Unknown" );
  recordZoneMapping( 35, "⇧" );
  recordZoneMapping( 36, "1" );
  recordZoneMapping( 37, "2" );
  recordZoneMapping( 38, "3" );
  recordZoneMapping( 39, "↵" );
  recordZoneMapping( 40, "Unused/Unknown" );
  recordZoneMapping( 41, "Unused/Unknown" );
  recordZoneMapping( 42, "Caps" );
  recordZoneMapping( 43, "Unused/Unknown" );
  recordZoneMapping( 44, "A" );
  recordZoneMapping( 45, "S" );
  recordZoneMapping( 46, "D" );
  recordZoneMapping( 47, "F" );
  recordZoneMapping( 48, "G" );
  recordZoneMapping( 49, "H" );
  recordZoneMapping( 50, "J" );
  recordZoneMapping( 51, "K" );
  recordZoneMapping( 52, "L" );
  recordZoneMapping( 53, "Ö" );
  recordZoneMapping( 54, "Ä" );
  recordZoneMapping( 55, "#" );
  recordZoneMapping( 56, "Unused/Unknown" );
  recordZoneMapping( 57, "4" );
  recordZoneMapping( 58, "5" );
  recordZoneMapping( 59, "6" );
  recordZoneMapping( 60, "Unused/Unknown" );
  recordZoneMapping( 61, "Unused/Unknown" );
  recordZoneMapping( 62, "Unused/Unknown" );
  recordZoneMapping( 63, "Tab" );
  recordZoneMapping( 64, "Unused/Unknown" );
  recordZoneMapping( 65, "Q" );
  recordZoneMapping( 66, "W" );
  recordZoneMapping( 67, "E" );
  recordZoneMapping( 68, "R" );
  recordZoneMapping( 69, "T" );
  recordZoneMapping( 70, "Z" );
  recordZoneMapping( 71, "U" );
  recordZoneMapping( 72, "I" );
  recordZoneMapping( 73, "O" );
  recordZoneMapping( 74, "P" );
  recordZoneMapping( 75, "Ü" );
  recordZoneMapping( 76, "+" );
  recordZoneMapping( 77, "↵" );
  recordZoneMapping( 78, "7" );
  recordZoneMapping( 79, "8" );
  recordZoneMapping( 80, "9" );
  recordZoneMapping( 81, "+" );
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
  recordZoneMapping( 98, "⌫" );
  recordZoneMapping( 99, "Num" );
  recordZoneMapping( 100, "/" );
  recordZoneMapping( 101, "*" );
  recordZoneMapping( 102, "-" );
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
  recordZoneMapping( 118, "PrtSc" );
  recordZoneMapping( 119, "Ins" );
  recordZoneMapping( 120, "Del" );
  recordZoneMapping( 121, "Home" );
  recordZoneMapping( 122, "End" );
  recordZoneMapping( 123, "PgUp" );
  recordZoneMapping( 124, "PgDn" );
  recordZoneMapping( 125, "Unused/Unknown" );

  // Initialize m_keys with default entries for all zones
  m_keys.resize( m_zones );
  for ( int i = 0; i < m_zones; ++i )
  {
    m_keys[i].zoneId = i;
    m_keys[i].label = QString( "Zone %1" ).arg( i );
    m_keys[i].color = Qt::white;
    m_keys[i].brightness = 255;
  }

  setupKeyboardLayout();

  // Initialize color dialog
  m_colorDialog = new QColorDialog( this );
  m_colorDialog->setOption( QColorDialog::ShowAlphaChannel, false );
  m_colorDialog->setOption( QColorDialog::DontUseNativeDialog, false );

  connect( m_colorDialog, &QColorDialog::colorSelected, this, &KeyboardVisualizerWidget::onColorChanged );
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
  m_layout->setSpacing( 2 );
  m_layout->setContentsMargins( 10, 10, 10, 10 );

  // Create a realistic German QWERTZ keyboard layout
  // Row 0: Esc + F1-F12 + PrintScreen + Insert + Delete
  createKeyButton( 105, getKeyLabel(105), 0, 0 );  // Esc
  for ( int i = 0; i < 12; ++i )
  {
    createKeyButton( 106 + i, getKeyLabel(106 + i), 0, i + 1 );  // F1-F12
  }
  createKeyButton( 118, getKeyLabel(118), 0, 13 ); // PrintScreen
  createKeyButton( 119, getKeyLabel(119), 0, 14 ); // Insert
  createKeyButton( 120, getKeyLabel(120), 0, 15 ); // Delete
  createKeyButton( 121, getKeyLabel(121), 0, 17 ); // Home
  createKeyButton( 122, getKeyLabel(122), 0, 19 ); // End
  createKeyButton( 123, getKeyLabel(123), 0, 16 ); // Page Up
  createKeyButton( 124, getKeyLabel(124), 0, 18 ); // Page Down

  // Row 1: ^ 1 2 3 4 5 6 7 8 9 0 ß ' + Backspace
  createKeyButton( 84, getKeyLabel(84), 1, 0 );   // ^
  for ( int i = 0; i < 12; ++i )
  {
    createKeyButton( 85 + i, getKeyLabel(85 + i), 1, i + 1 );  // 1-0 ß '
  }
  createKeyButton( 98, getKeyLabel(98), 1, 13, 2, 1 );  // Backspace (wider)

  // Row 2: Tab + q w e r t z u i o p ü +
  createKeyButton( 63, getKeyLabel(63), 2, 0, 2, 1 );  // Tab (wider)
  createKeyButton( 65, getKeyLabel(65), 2, 2 );  // q
  createKeyButton( 66, getKeyLabel(66), 2, 3 );  // w
  createKeyButton( 67, getKeyLabel(67), 2, 4 );  // e
  createKeyButton( 68, getKeyLabel(68), 2, 5 );  // r
  createKeyButton( 69, getKeyLabel(69), 2, 6 );  // t
  createKeyButton( 70, getKeyLabel(70), 2, 7 );  // z
  createKeyButton( 71, getKeyLabel(71), 2, 8 );  // u
  createKeyButton( 72, getKeyLabel(72), 2, 9 );  // i
  createKeyButton( 73, getKeyLabel(73), 2, 10 ); // o
  createKeyButton( 74, getKeyLabel(74), 2, 11 ); // p
  createKeyButton( 75, getKeyLabel(75), 2, 12 ); // ü
  createKeyButton( 76, getKeyLabel(76), 2, 13 ); // +

  // Row 3: Caps + a s d f g h j k l ö ä # + Enter
  createKeyButton( 42, getKeyLabel(42), 3, 0, 2, 1 );  // Caps Lock (wider)
  createKeyButton( 44, getKeyLabel(44), 3, 2 );  // a
  createKeyButton( 45, getKeyLabel(45), 3, 3 );  // s
  createKeyButton( 46, getKeyLabel(46), 3, 4 );  // d
  createKeyButton( 47, getKeyLabel(47), 3, 5 );  // f
  createKeyButton( 48, getKeyLabel(48), 3, 6 );  // g
  createKeyButton( 49, getKeyLabel(49), 3, 7 );  // h
  createKeyButton( 50, getKeyLabel(50), 3, 8 );  // j
  createKeyButton( 51, getKeyLabel(51), 3, 9 );  // k
  createKeyButton( 52, getKeyLabel(52), 3, 10 ); // l
  createKeyButton( 53, getKeyLabel(53), 3, 11 ); // ö
  createKeyButton( 54, getKeyLabel(54), 3, 12 ); // ä
  createKeyButton( 55, getKeyLabel(55), 3, 13 ); // #
  createKeyButton( 77, getKeyLabel(77), 2, 14, 1, 2 );  // Enter (taller, spans 2 cols)

  // Row 4: Left Shift + < y x c v b n m , . - + Right Shift
  createKeyButton( 22, getKeyLabel(22), 4, 0, 1, 1 );  // Left Shift
  createKeyButton( 23, getKeyLabel(23), 4, 1 );  // <
  createKeyButton( 24, getKeyLabel(24), 4, 2 );  // y
  createKeyButton( 25, getKeyLabel(25), 4, 3 );  // x
  createKeyButton( 26, getKeyLabel(26), 4, 4 );  // c
  createKeyButton( 27, getKeyLabel(27), 4, 5 );  // v
  createKeyButton( 28, getKeyLabel(28), 4, 6 );  // b
  createKeyButton( 29, getKeyLabel(29), 4, 7 );  // n
  createKeyButton( 30, getKeyLabel(30), 4, 8 );  // m
  createKeyButton( 31, getKeyLabel(31), 4, 9 ); // ,
  createKeyButton( 32, getKeyLabel(32), 4, 10 ); // .
  createKeyButton( 33, getKeyLabel(33), 4, 11 ); // -
  createKeyButton( 35, getKeyLabel(35), 4, 12, 3, 1 );  // Right Shift (wider)

  // Row 5: Left Ctrl + Fn + Left Win + Left Alt + Space + Right Alt + Right Win + Menu + Right Ctrl
  createKeyButton( 0, getKeyLabel(0), 5, 0 );    // Left Ctrl
  createKeyButton( 2, getKeyLabel(2), 5, 1 );    // Fn
  createKeyButton( 3, getKeyLabel(3), 5, 2 );    // Left Windows
  createKeyButton( 4, getKeyLabel(4), 5, 3 );    // Left Alt
  createKeyButton( 7, getKeyLabel(7), 5, 4, 5, 1 );  // Space bar (much wider to fill gap)
  createKeyButton( 10, getKeyLabel(10), 5, 9 );   // Right Alt
  createKeyButton( 12, getKeyLabel(12), 5, 10 );  // Right Ctrl

  // Numpad (right side) - moved up one row
  createKeyButton( 99, getKeyLabel(99), 1, 16 );  // Num Lock
  createKeyButton( 100, getKeyLabel(100), 1, 17 ); // Numpad /
  createKeyButton( 101, getKeyLabel(101), 1, 18 ); // Numpad *
  createKeyButton( 102, getKeyLabel(102), 1, 19 ); // Numpad -

  createKeyButton( 78, getKeyLabel(78), 2, 16 );  // Numpad 7
  createKeyButton( 79, getKeyLabel(79), 2, 17 );  // Numpad 8
  createKeyButton( 80, getKeyLabel(80), 2, 18 );  // Numpad 9
  createKeyButton( 81, getKeyLabel(81), 2, 19, 1, 2 ); // Numpad + (taller)

  createKeyButton( 57, getKeyLabel(57), 3, 16 );  // Numpad 4
  createKeyButton( 58, getKeyLabel(58), 3, 17 );  // Numpad 5
  createKeyButton( 59, getKeyLabel(59), 3, 18 );  // Numpad 6

  createKeyButton( 36, getKeyLabel(36), 4, 16 );  // Numpad 1
  createKeyButton( 37, getKeyLabel(37), 4, 17 );  // Numpad 2
  createKeyButton( 38, getKeyLabel(38), 4, 18 );  // Numpad 3
  createKeyButton( 39, getKeyLabel(39), 4, 19, 1, 2 ); // Numpad Enter (taller)

  createKeyButton( 16, getKeyLabel(16), 5, 20, 2, 1 ); // Numpad 0 (wider)
  createKeyButton( 17, getKeyLabel(17), 5, 18 );  // Numpad ,

  // Navigation keys (arrow keys under right shift)
  createKeyButton( 14, getKeyLabel(14), 5, 13 );  // Up Arrow
  createKeyButton( 13, getKeyLabel(13), 6, 12 );  // Left Arrow
  createKeyButton( 18, getKeyLabel(18), 6, 13 );  // Down Arrow
  createKeyButton( 15, getKeyLabel(15), 6, 14 );  // Right Arrow

  m_scrollArea->setWidget( m_keyboardWidget );
  mainLayout->addWidget( m_scrollArea );

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

  // Update key info (m_keys is already initialized)
  if ( zoneId >= 0 && zoneId < static_cast< int >( m_keys.size() ) )
  {
    m_keys[zoneId].label = label;
    m_keys[zoneId].color = Qt::white;
    m_keys[zoneId].brightness = 255;
  }

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

void KeyboardVisualizerWidget::recordZoneMapping( int zoneId, const QString &keyName )
{
  m_zoneMappings[zoneId] = keyName;
}

QString KeyboardVisualizerWidget::getKeyLabel( int zoneId ) const
{
  auto it = m_zoneMappings.find( zoneId );
  if ( it != m_zoneMappings.end() )
  {
    return it->second;
  }
  return QString( "Zone %1" ).arg( zoneId );
}

} // namespace ucc