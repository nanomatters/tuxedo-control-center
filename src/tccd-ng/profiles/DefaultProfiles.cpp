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

#include "DefaultProfiles.hpp"
#include <iostream>

const TccProfile maxEnergySave = []()
{
  TccProfile profile( DefaultProfileIDs::MaxEnergySave, DefaultProfileIDs::MaxEnergySave );

  profile.display.brightness = 40;
  profile.display.useBrightness = true;
  profile.display.refreshRate = 60;
  profile.display.useRefRate = false;
  profile.display.xResolution = 1920;
  profile.display.yResolution = 1080;
  profile.display.useResolution = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Silent";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  profile.odmProfile.name = "power_save";
  profile.odmPowerLimits.tdpValues = { 5, 10, 15 };
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(0);

  return profile;
}();

const TccProfile silent = []()
{
  TccProfile profile( DefaultProfileIDs::Quiet, DefaultProfileIDs::Quiet );

  profile.display.brightness = 50;
  profile.display.useBrightness = true;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Silent";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  profile.odmProfile.name = "power_save";
  profile.odmPowerLimits.tdpValues = { 10, 15, 25 };
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(0);

  return profile;
}();

const TccProfile office = []()
{
  TccProfile profile( DefaultProfileIDs::Office, DefaultProfileIDs::Office );

  profile.display.brightness = 60;
  profile.display.useBrightness = true;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Quiet";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  profile.odmProfile.name = "enthusiast";
  profile.odmPowerLimits.tdpValues = { 25, 35, 35 };
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(0);

  return profile;
}();

const TccProfile highPerformance = []()
{
  TccProfile profile( DefaultProfileIDs::HighPerformance, DefaultProfileIDs::HighPerformance );

  profile.display.brightness = 60;
  profile.display.useBrightness = true;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Balanced";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  profile.odmProfile.name = "overboost";
  profile.odmPowerLimits.tdpValues = { 60, 60, 70 };
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(0);

  return profile;
}();

const TccProfile defaultCustomProfile = []()
{
  TccProfile profile( defaultCustomProfileID, "TUXEDO Defaults" );
  profile.description = "Edit profile to change behaviour";

  profile.display.brightness = 100;
  profile.display.useBrightness = false;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Balanced";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  // odmProfile.name is optional, leave unset
  // odmPowerLimits.tdpValues is empty
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(0);

  return profile;
}();

const TccProfile defaultMobileCustomProfileTDP = []()
{
  TccProfile profile( defaultMobileCustomProfileID, "TUXEDO Mobile Default" );
  profile.description = "Edit profile to change behaviour";

  profile.display.brightness = 100;
  profile.display.useBrightness = false;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.scalingMaxFrequency = 3500000;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Balanced";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  // odmProfile.name is optional, leave unset
  profile.odmPowerLimits.tdpValues = { 15, 25, 50 };
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(0);

  return profile;
}();

const TccProfile defaultMobileCustomProfileCl = []()
{
  TccProfile profile( defaultMobileCustomProfileID, "TUXEDO Mobile Default" );
  profile.description = "Edit profile to change behaviour";

  profile.display.brightness = 100;
  profile.display.useBrightness = false;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.scalingMaxFrequency = 3500000;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Balanced";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  // odmProfile.name is optional, leave unset
  // odmPowerLimits.tdpValues is empty
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(0);

  return profile;
}();

const TccProfile highPerformance25WcTGP = []()
{
  TccProfile profile( DefaultProfileIDs::HighPerformance, DefaultProfileIDs::HighPerformance );

  profile.display.brightness = 60;
  profile.display.useBrightness = true;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Balanced";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  profile.odmProfile.name = "overboost";
  profile.odmPowerLimits.tdpValues = { 60, 60, 70 };
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(25);

  return profile;
}();

const TccProfile defaultCustomProfile25WcTGP = []()
{
  TccProfile profile( defaultCustomProfileID, "TUXEDO Defaults" );
  profile.description = "Edit profile to change behaviour";

  profile.display.brightness = 100;
  profile.display.useBrightness = false;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Balanced";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  // odmProfile.name is optional, leave unset
  // odmPowerLimits.tdpValues is empty
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(25);

  return profile;
}();

// Legacy default profiles (for devices not in deviceProfiles map)
const TccProfile legacyDefault = []()
{
  TccProfile profile( "__legacy_default__", "Default" );

  profile.display.brightness = 100;
  profile.display.useBrightness = false;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Balanced";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  // odmProfile.name is optional, leave unset
  profile.odmPowerLimits.tdpValues = { 25, 35, 35 };
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(0);

  return profile;
}();

const TccProfile legacyCoolAndBreezy = []()
{
  TccProfile profile( "__legacy_cool_and_breezy__", "Cool and breezy" );

  profile.display.brightness = 50;
  profile.display.useBrightness = false;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "balance_performance";
  profile.cpu.noTurbo = false;

  profile.webcam.status = true;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Quiet";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  // odmProfile.name is optional, leave unset
  profile.odmPowerLimits.tdpValues = { 10, 15, 25 };
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(0);

  return profile;
}();

const TccProfile legacyPowersaveExtreme = []()
{
  TccProfile profile( "__legacy_powersave_extreme__", "Powersave extreme" );

  profile.display.brightness = 30;
  profile.display.useBrightness = false;
  profile.display.refreshRate = -1;
  profile.display.useRefRate = false;

  profile.cpu.useMaxPerfGov = false;
  profile.cpu.governor = "powersave";
  profile.cpu.energyPerformancePreference = "power";
  profile.cpu.noTurbo = true;

  profile.webcam.status = false;
  profile.webcam.useStatus = true;

  profile.fan.useControl = true;
  profile.fan.fanProfile = "Silent";
  profile.fan.minimumFanspeed = 0;
  profile.fan.maximumFanspeed = 100;
  profile.fan.offsetFanspeed = 0;
  profile.fan.customFanCurve = customFanPreset;

  // odmProfile.name is optional, leave unset
  profile.odmPowerLimits.tdpValues = { 5, 10, 15 };
  profile.nvidiaPowerCTRLProfile = TccNVIDIAPowerCTRLProfile(0);

  return profile;
}();

const std::vector< TccProfile > legacyProfiles = { legacyDefault, legacyCoolAndBreezy, legacyPowersaveExtreme };

// device-specific default profiles
const std::map< TUXEDODevice, std::vector< TccProfile > > deviceProfiles = 
{
  { TUXEDODevice::IBP14G6_TUX, { maxEnergySave, silent, office } },
  { TUXEDODevice::IBP14G6_TRX, { maxEnergySave, silent, office } },
  { TUXEDODevice::IBP14G6_TQF, { maxEnergySave, silent, office } },
  { TUXEDODevice::IBP14G7_AQF_ARX, { maxEnergySave, silent, office } },
  { TUXEDODevice::IBPG8, { maxEnergySave, silent, office } },

  { TUXEDODevice::PULSE1502, { maxEnergySave, silent, office } },

  { TUXEDODevice::POLARIS1XI02, { maxEnergySave, silent, office, highPerformance } },
  { TUXEDODevice::POLARIS1XI03, { maxEnergySave, silent, office, highPerformance } },
  { TUXEDODevice::POLARIS1XA02, { maxEnergySave, silent, office, highPerformance25WcTGP } },
  { TUXEDODevice::POLARIS1XA03, { maxEnergySave, silent, office, highPerformance25WcTGP } },
  { TUXEDODevice::POLARIS1XA05, { maxEnergySave, silent, office, highPerformance } },

  { TUXEDODevice::STELLARIS1XI03, { maxEnergySave, silent, office, highPerformance } },
  { TUXEDODevice::STELLARIS1XI04, { maxEnergySave, silent, office, highPerformance } },
  { TUXEDODevice::STELLARIS1XI05, { maxEnergySave, silent, office, highPerformance } },

  { TUXEDODevice::STELLARIS1XA03, { maxEnergySave, silent, office, highPerformance } },
  { TUXEDODevice::STEPOL1XA04, { maxEnergySave, silent, office, highPerformance } },
  { TUXEDODevice::STELLARIS1XA05, { maxEnergySave, silent, office, highPerformance } },

  { TUXEDODevice::STELLARIS16I06, { maxEnergySave, silent, office, highPerformance } },
  { TUXEDODevice::STELLARIS17I06, { maxEnergySave, silent, office, highPerformance } },
};

// device-specific custom profile defaults
const std::map< TUXEDODevice, std::vector< TccProfile > > deviceCustomProfiles =
{
  // devices not listed here default to [ defaultCustomProfile ]
  // the first entry is used as the skeleton for new profiles created by the user
  { TUXEDODevice::IBPG8, { defaultCustomProfile, defaultMobileCustomProfileTDP } },
  { TUXEDODevice::AURA14G3, { defaultCustomProfile, defaultMobileCustomProfileCl } },
  { TUXEDODevice::AURA15G3, { defaultCustomProfile, defaultMobileCustomProfileCl } },
  { TUXEDODevice::POLARIS1XA02, { defaultCustomProfile25WcTGP } },
  { TUXEDODevice::POLARIS1XA03, { defaultCustomProfile25WcTGP } },
  { TUXEDODevice::STELLARIS1XA03, { defaultCustomProfile25WcTGP } },
};
