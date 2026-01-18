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

#include <memory>
#include <functional>
#include "DaemonWorker.hpp"
#include "tuxedo_io_lib/tuxedo_io_api.hh"

// Forward declarations
class TccDBusData;

/**
 * @brief Webcam Worker
 *
 * Manages webcam switch status monitoring and control.
 * Monitors hardware webcam switch availability and status,
 * updating DBus data accordingly.
 *
 * This worker:
 *   - Detects webcam switch hardware availability
 *   - Monitors webcam on/off status
 *   - Applies webcam settings from active profile
 *   - Periodically updates webcam status
 */
class WebcamWorker : public DaemonWorker
{
public:
  /**
   * @brief Constructor
   *
   * @param dbusData Reference to shared DBus data container (includes mutex)
   * @param io Reference to TuxedoIOAPI instance
   */
  WebcamWorker( TccDBusData &dbusData, TuxedoIOAPI &io );

  virtual ~WebcamWorker() = default;

  // Prevent copy and move
  WebcamWorker( const WebcamWorker & ) = delete;
  WebcamWorker( WebcamWorker && ) = delete;
  WebcamWorker &operator=( const WebcamWorker & ) = delete;
  WebcamWorker &operator=( WebcamWorker && ) = delete;

  /**
   * @brief Set callback to retrieve active profile webcam settings
   *
   * The callback should return a pair of:
   *   - bool: whether to use/apply the webcam setting
   *   - bool: desired webcam status (true = on, false = off)
   *
   * @param callback Function that returns webcam profile settings
   */
  void setProfileCallback( std::function< std::pair< bool, bool >() > callback )
  {
    m_profileCallback = std::move( callback );
  }

protected:
  void onStart() override;
  void onWork() override;
  void onExit() override;

private:
  TccDBusData &m_dbusData;
  TuxedoIOAPI &m_io;
  std::function< std::pair< bool, bool >() > m_profileCallback;

  /**
   * @brief Update webcam status in DBus data
   *
   * Queries hardware for webcam switch availability and current status,
   * updating the DBus data structure accordingly.
   */
  void updateWebcamStatuses();

  /**
   * @brief Apply webcam settings from active profile
   *
   * If a profile callback is set, retrieves the profile's webcam settings
   * and applies them to the hardware.
   */
  void applyProfileSettings();
};
