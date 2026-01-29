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

#include "TccProfile.hpp"
#include <string>
#include <vector>
#include <map>

// default custom profile IDs
inline constexpr const char *defaultCustomProfileID = "__default_custom_profile__";
inline constexpr const char *defaultMobileCustomProfileID = "__default_mobile_custom_profile__";

/**
 * @brief TUXEDO device types
 *
 * Enumeration of supported TUXEDO devices for device-specific profile handling
 */
enum class TUXEDODevice
{
  IBP17G6,
  PULSE1403,
  PULSE1404,
  IBP14G6_TUX,
  IBP14G6_TRX,
  IBP14G6_TQF,
  IBP14G7_AQF_ARX,
  IBPG8,
  IBPG10AMD,
  PULSE1502,
  AURA14G3,
  AURA15G3,
  POLARIS1XA02,
  POLARIS1XI02,
  POLARIS1XA03,
  POLARIS1XI03,
  POLARIS1XA05,
  STELLARIS1XA03,
  STELLARIS1XI03,
  STELLARIS1XI04,
  STEPOL1XA04,
  STELLARIS1XI05,
  STELLARIS1XA05,
  STELLARIS16I06,
  STELLARIS17I06,
  STELLSL15A06,
  STELLSL15I06,
  STELLARIS16A07,
  STELLARIS16I07,
  SIRIUS1601,
  SIRIUS1602,
};

// pre-defined profiles
extern const TccProfile maxEnergySave;
extern const TccProfile silent;
extern const TccProfile office;
extern const TccProfile highPerformance;
extern const TccProfile highPerformance25WcTGP;

// legacy profiles (for unknown devices)
extern const TccProfile legacyDefault;
extern const TccProfile legacyCoolAndBreezy;
extern const TccProfile legacyPowersaveExtreme;
extern const std::vector< TccProfile > legacyProfiles;

// default custom profiles
extern const TccProfile defaultCustomProfile;
extern const TccProfile defaultMobileCustomProfileTDP;
extern const TccProfile defaultMobileCustomProfileCl;
extern const TccProfile defaultCustomProfile25WcTGP;

// device-specific profile mappings
extern const std::map< TUXEDODevice, std::vector< TccProfile > > deviceProfiles;
extern const std::map< TUXEDODevice, std::vector< TccProfile > > deviceCustomProfiles;

