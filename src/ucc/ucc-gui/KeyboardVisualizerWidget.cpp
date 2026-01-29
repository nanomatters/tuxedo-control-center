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

  // For 126 zones, create a grid layout that shows zones in a more organized way
  // This avoids assuming a specific physical keyboard layout
  int currentZone = 0;
  int cols = 16;  // 16 columns for a reasonable width
  int rows = (m_zones + cols - 1) / cols;  // Calculate rows needed

  for ( int row = 0; row < rows && currentZone < m_zones; ++row )
  {
    for ( int col = 0; col < cols && currentZone < m_zones; ++col )
    {
      QString label = QString::number( currentZone );
      createKeyButton( currentZone++, label, row, col );
    }
  }

  m_scrollArea->setWidget( m_keyboardWidget );
  mainLayout->addWidget( m_scrollArea );

  // Instructions label
  QLabel *instructions = new QLabel( "Click on zone numbers to select and change their color. Use global controls for all zones." );
  instructions->setStyleSheet( "font-size: 11px; color: #888; margin-top: 5px;" );
  instructions->setWordWrap( true );
  mainLayout->addWidget( instructions );
}

void KeyboardVisualizerWidget::createKeyButton( int zoneId, const QString &label, int row, int col, int width, int height )
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

void KeyboardVisualizerWidget::onKeyClicked()
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

void KeyboardVisualizerWidget::onColorChanged( const QColor &color )
{
  if ( m_selectedZoneId >= 0 && m_selectedZoneId < static_cast< int >( m_keys.size() ) )
  {
    m_keys[m_selectedZoneId].color = color;
    updateKeyAppearance( m_selectedButton, color, m_keys[m_selectedZoneId].brightness );
    emit colorsChanged();
  }
}

void KeyboardVisualizerWidget::updateKeyAppearance( QPushButton *button, const QColor &color, int brightness )
{
  QColor adjustedColor = applyBrightness( color, brightness );
  QString style = QString( "background-color: %1; color: %2; border: 1px solid #666; border-radius: 3px;" )
                  .arg( adjustedColor.name() )
                  .arg( adjustedColor.lightness() > 128 ? "#000000" : "#ffffff" );
  button->setStyleSheet( style );
}

QColor KeyboardVisualizerWidget::applyBrightness( const QColor &color, int brightness ) const
{
  // Scale brightness from 0-255 to 0.0-1.0
  double factor = brightness / 255.0;

  int r = qMin( 255, static_cast< int >( color.red() * factor ) );
  int g = qMin( 255, static_cast< int >( color.green() * factor ) );
  int b = qMin( 255, static_cast< int >( color.blue() * factor ) );

  return QColor( r, g, b );
}

void KeyboardVisualizerWidget::loadCurrentStates( const std::string &statesJSON )
{
  QJsonDocument doc = QJsonDocument::fromJson( QString::fromStdString( statesJSON ).toUtf8() );
  if ( doc.isArray() )
  {
    updateFromJSON( doc.array() );
  }
}

void KeyboardVisualizerWidget::updateFromJSON( const QJsonArray &states )
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

QJsonArray KeyboardVisualizerWidget::getJSONState() const
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

void KeyboardVisualizerWidget::setGlobalBrightness( int brightness )
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

void KeyboardVisualizerWidget::setGlobalColor( const QColor &color )
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

} // namespace ucc