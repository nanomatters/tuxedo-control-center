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

#include "WebcamWorker.hpp"
#include "../TccDBusService.hpp"
#include <iostream>
#include <syslog.h>

WebcamWorker::WebcamWorker( TccDBusData &dbusData, TuxedoIOAPI &io )
  : DaemonWorker( std::chrono::milliseconds( 2000 ), false ),
    m_dbusData( dbusData ),
    m_io( io ),
    m_profileCallback( nullptr )
{
}

void WebcamWorker::onStart()
{
  syslog( LOG_INFO, "WebcamWorker: Starting" );

  // initial status update
  updateWebcamStatuses();

  // apply profile settings if available
  applyProfileSettings();

  // update status again after applying settings
  updateWebcamStatuses();
}

void WebcamWorker::onWork()
{
  updateWebcamStatuses();
}

void WebcamWorker::onExit()
{
  // no cleanup needed
}

void WebcamWorker::updateWebcamStatuses()
{
  bool status = false;
  bool available = m_io.getWebcam( status );

  std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );
  m_dbusData.webcamSwitchAvailable = available;
  m_dbusData.webcamSwitchStatus = status;

  syslog( LOG_DEBUG, "WebcamWorker: Webcam available=%d, status=%d",
          available, status );
}

void WebcamWorker::applyProfileSettings()
{
  // check if we have a profile callback
  if ( not m_profileCallback )
    return;

  // get profile settings
  auto [useStatus, desiredStatus] = m_profileCallback();

  // check if webcam switch is available
  std::lock_guard< std::mutex > lock( m_dbusData.dataMutex );

  // always force webcam to selected setting
  // (option to not set is removed for now, matching TypeScript behavior)
  if ( m_dbusData.webcamSwitchAvailable && ( true or useStatus ) )
  {
    syslog( LOG_INFO, "WebcamWorker: Setting webcam status %s", desiredStatus ? "ON" : "OFF" );

    if ( not m_io.setWebcam( desiredStatus ) )
      syslog( LOG_WARNING, "WebcamWorker: Failed to change webcam status" );
  }
}
