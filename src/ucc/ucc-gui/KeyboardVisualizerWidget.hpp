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

#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QColor>
#include <QColorDialog>
#include <QLabel>
#include <QScrollArea>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <vector>
#include <map>

namespace ucc
{

/**
 * @brief Structure representing a keyboard key
 */
struct KeyboardKey
{
  int zoneId = -1;
  QString label;
  QRect geometry;  // Position and size in the layout
  QColor color = Qt::white;
  int brightness = 255;
};

/**
 * @brief Widget for visualizing and controlling per-key keyboard backlight
 */
class KeyboardVisualizerWidget : public QWidget
{
  Q_OBJECT

public:
  explicit KeyboardVisualizerWidget( int zones, QWidget *parent = nullptr );
  ~KeyboardVisualizerWidget() override = default;

  /**
   * @brief Load current keyboard backlight states from daemon
   */
  void loadCurrentStates( const std::string &statesJSON );

  /**
   * @brief Update the colors of keys from JSON state array
   */
  void updateFromJSON( const QJsonArray &states );

  /**
   * @brief Get current keyboard state as JSON array
   */
  QJsonArray getJSONState() const;

  /**
   * @brief Set global brightness for all keys
   */
  void setGlobalBrightness( int brightness );

  /**
   * @brief Set global color for all keys
   */
  void setGlobalColor( const QColor &color );

  /**
   * @brief Start zone mapping mode - set all to min brightness
   */
  void startZoneMapping();

  /**
   * @brief Test a specific zone by setting it to max brightness
   */
  void testZone( int zoneId );

  /**
   * @brief Get next zone to test
   */
  int getNextTestZone();
  /**
   * @brief Record a zone-to-key mapping
   */
  void recordZoneMapping( int zoneId, const QString &keyName );

  /**
   * @brief Get the key label for a zone ID
   */
  QString getKeyLabel( int zoneId ) const;

  /**
   * @brief Get the current zone mappings as a string
   */
  QString getZoneMappings() const;
signals:
  /**
   * @brief Emitted when a key is selected
   */
  void keySelected( int zoneId );

  /**
   * @brief Emitted when key colors change
   */
  void colorsChanged();

private slots:
  void onKeyClicked();
  void onColorChanged( const QColor &color );
  void onTestNextZone();

private:
  void setupKeyboardLayout();
  void createKeyButton( int zoneId, const QString &label, int row, int col, int width = 1, int height = 1 );
  QColor applyBrightness( const QColor &color, int brightness ) const;
  void updateKeyAppearance( QPushButton *button, const QColor &color, int brightness );

  int m_zones;
  std::vector< KeyboardKey > m_keys;
  QGridLayout *m_layout = nullptr;
  QScrollArea *m_scrollArea = nullptr;
  QWidget *m_keyboardWidget = nullptr;

  // Selected key tracking
  int m_selectedZoneId = -1;
  QPushButton *m_selectedButton = nullptr;

  // Color picker dialog
  QColorDialog *m_colorDialog = nullptr;

  // Zone mapping controls
  QPushButton *m_startMappingButton = nullptr;
  QPushButton *m_testNextZoneButton = nullptr;
  QLabel *m_mappingStatusLabel = nullptr;
  int m_currentTestZone = 0;
  bool m_mappingMode = false;
  std::map<int, QString> m_zoneMappings;
};

} // namespace ucc