/*!
 * Copyright (c) 2020-2022 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 *
 * This file is part of TUXEDO Control Center.
 *
 * TUXEDO Control Center is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TUXEDO Control Center is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TUXEDO Control Center.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include "interface.h"
#include <vector>
#include <string>

/*!
 * Null Object implementation of DeviceInterface
 * Used when no actual device is detected, provides safe fallback behavior
 */
class DummyDevice : public DeviceInterface
{
public:
  DummyDevice(IO &io) : DeviceInterface(io)
  {
  }

  static inline bool canIdentify(IO &io, bool &identified)
  {
    identified = false;
    return false;
  }

  virtual bool identify( bool &identified )
  { return canIdentify( m_io, identified ); }

  virtual bool deviceInterfaceIdStr( std::string &interfaceIdStr )
  { interfaceIdStr = "unknown"; return false; }

  virtual bool deviceModelIdStr( std::string &modelIdStr )
  { modelIdStr = "unknown"; return false; }

  virtual bool setEnableModeSet( bool enabled )
  { return false; }

  virtual bool getFansMinSpeed( int &minSpeed )
  { minSpeed = 0; return false; }

  virtual bool getFansOffAvailable( bool &offAvailable )
  { offAvailable = false; return false; }

  virtual bool getNumberFans( int &nrFans )
  { nrFans = 0; return false; }

  virtual bool setFansAuto()
  { return false; }

  virtual bool setFanSpeedPercent( const int fanNr, const int fanSpeedPercent )
  { return false; }

  virtual bool getFanSpeedPercent( const int fanNr, int &fanSpeedPercent )
  { fanSpeedPercent = 0; return false; }

  virtual bool getFanTemperature( const int fanNr, int &temperatureCelcius )
  { temperatureCelcius = 0; return false; }

  virtual bool setWebcam( const bool status )
  { return false; }

  virtual bool getWebcam( bool &status )
  { status = false; return false; }

  virtual bool getAvailableODMPerformanceProfiles( std::vector< std::string > &profiles )
  { profiles.clear(); return false; }

  virtual bool setODMPerformanceProfile( std::string performanceProfile )
  { return false; }

  virtual bool getDefaultODMPerformanceProfile( std::string &profileName )
  { profileName = "unknown"; return false; }

  virtual bool getNumberTDPs( int &nrTDPs )
  { nrTDPs = 0; return false; }

  virtual bool getTDPDescriptors( std::vector< std::string > &tdpDescriptors )
  { tdpDescriptors.clear(); return false; }

  virtual bool getTDPMin( const int tdpIndex, int &minValue )
  { minValue = 0; return false; }

  virtual bool getTDPMax( const int tdpIndex, int &maxValue )
  { maxValue = 0; return false; }

  virtual bool setTDP( const int tdpIndex, int tdpValue )
  { return false; }

  virtual bool getTDP( const int tdpIndex, int &tdpValue )
  { tdpValue = 0; return false; }
};
