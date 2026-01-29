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

#include "DaemonWorker.hpp"
#include "GpuInfoWorker.hpp"
#include <string>
#include <optional>
#include <chrono>
#include <sstream>

/**
 * @brief Worker for CPU power monitoring
 *
 * Reads Intel RAPL power data and updates cpuPowerValuesJSON for GUI display.
 * Polls every 2000ms.
 * Uses existing IntelRAPLController and PowerController from GpuInfoWorker.hpp.
 */
class CpuPowerWorker : public DaemonWorker
{
private:
  bool m_RAPLConstraint0Status = false;
  bool m_RAPLConstraint1Status = false;
  bool m_RAPLConstraint2Status = false;

  std::unique_ptr< IntelRAPLController > m_intelRAPL;
  std::unique_ptr< PowerController > m_powerController;

  std::function< void( const std::string & ) > m_updateCallback;
  std::function< bool() > m_getSensorDataCollectionStatus;

public:
  explicit CpuPowerWorker(
    std::function< void( const std::string & ) > updateCallback,
    std::function< bool() > getSensorDataCollectionStatus = []() { return true; }
  )
    : DaemonWorker( std::chrono::milliseconds( 2000 ) )
    , m_updateCallback( updateCallback )
    , m_getSensorDataCollectionStatus( getSensorDataCollectionStatus )
  {}

  void onStart() override
  {
    m_intelRAPL = std::make_unique< IntelRAPLController >( "/sys/devices/virtual/powercap/intel-rapl/intel-rapl:0/" );
    m_intelRAPL->updateFromSysfs();
    m_powerController = std::make_unique< PowerController >( *m_intelRAPL );

    m_RAPLConstraint0Status = m_intelRAPL->getIntelRAPLConstraint0Available();
    m_RAPLConstraint1Status = m_intelRAPL->getIntelRAPLConstraint1Available();
    m_RAPLConstraint2Status = m_intelRAPL->getIntelRAPLConstraint2Available();

    onWork();
  }

  void onWork() override
  {
    std::ostringstream jsonStream;
    jsonStream << "{";

    if ( m_getSensorDataCollectionStatus() )
    {
      double powerDraw = getCurrentPower();
      jsonStream << "\"powerDraw\":" << powerDraw;

      double maxPowerLimit = getMaxPowerLimit();
      if ( maxPowerLimit > 0 )
      {
        jsonStream << ",\"maxPowerLimit\":" << maxPowerLimit;
      }
    }
    else
    {
      jsonStream << "\"powerDraw\":-1";
    }

    jsonStream << "}";
    m_updateCallback( jsonStream.str() );
  }

  void onExit() override
  {
    m_powerController.reset();
    m_intelRAPL.reset();
  }

private:
  /**
   * @brief Get current CPU power draw in watts
   */
  double getCurrentPower()
  {
    if ( not m_powerController )
      return -1.0;

    return m_powerController->getCurrentPower();
  }

  /**
   * @brief Get maximum power limit from all available constraints
   *
   * Returns the highest power limit across constraint 0, 1, and 2 in watts.
   */
  double getMaxPowerLimit()
  {
    if ( not m_RAPLConstraint0Status
         and not m_RAPLConstraint1Status
         and not m_RAPLConstraint2Status )
    {
      return -1.0;
    }

    double maxPowerLimit = -1.0;

    if ( m_RAPLConstraint0Status )
    {
      int64_t constraint0MaxPower = m_intelRAPL->getConstraint0MaxPower();
      if ( constraint0MaxPower > 0 )
      {
        maxPowerLimit = static_cast< double >( constraint0MaxPower );
      }
    }

    if ( m_RAPLConstraint1Status )
    {
      int64_t constraint1MaxPower = m_intelRAPL->getConstraint1MaxPower();
      if ( constraint1MaxPower > 0 and static_cast< double >( constraint1MaxPower ) > maxPowerLimit )
      {
        maxPowerLimit = static_cast< double >( constraint1MaxPower );
      }
    }

    if ( m_RAPLConstraint2Status )
    {
      int64_t constraint2MaxPower = m_intelRAPL->getConstraint2MaxPower();
      if ( constraint2MaxPower > 0 and static_cast< double >( constraint2MaxPower ) > maxPowerLimit )
      {
        maxPowerLimit = static_cast< double >( constraint2MaxPower );
      }
    }

    // Convert from micro watts to watts
    return maxPowerLimit / 1000000.0;
  }
};
