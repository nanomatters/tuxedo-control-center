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

#include "TccdClient.hpp"
#include <QDBusMessage>
#include <QDBusError>
#include <QDebug>

namespace ucc
{

TccdClient::TccdClient( QObject *parent )
  : QObject( parent )
  , m_interface( std::make_unique< QDBusInterface >(
      DBUS_SERVICE,
      DBUS_PATH,
      DBUS_INTERFACE,
      QDBusConnection::systemBus(),
      this ) )
{
  m_connected = m_interface->isValid();

  if ( m_connected )
  {
    // Subscribe to signals
    QDBusConnection::systemBus().connect(
      DBUS_SERVICE,
      DBUS_PATH,
      DBUS_INTERFACE,
      "ProfileChanged",
      this,
      SLOT( onProfileChangedSignal( QString ) )
    );

    QDBusConnection::systemBus().connect(
      DBUS_SERVICE,
      DBUS_PATH,
      DBUS_INTERFACE,
      "PowerStateChanged",
      this,
      SLOT( onPowerStateChangedSignal( QString ) )
    );
  }
  else
  {
    qWarning() << "Failed to connect to tccd-ng DBus service:"
               << m_interface->lastError().message();
  }

  emit connectionStatusChanged( m_connected );
}

bool TccdClient::isConnected() const
{
  return m_connected && m_interface->isValid();
}

// Signal handlers
void TccdClient::onProfileChangedSignal( const QString &profileId )
{
  emit profileChanged( profileId );
}

void TccdClient::onPowerStateChangedSignal( const QString &state )
{
  emit powerStateChanged( state );
}

// Template implementations
template< typename T >
std::optional< T > TccdClient::callMethod( const QString &method ) const
{
  if ( !isConnected() )
  {
    return std::nullopt;
  }

  QDBusReply< T > reply = m_interface->call( method );
  if ( reply.isValid() )
  {
    return reply.value();
  }
  else
  {
    qWarning() << "DBus call failed:" << method << "-" << reply.error().message();
    return std::nullopt;
  }
}

template< typename T, typename... Args >
std::optional< T > TccdClient::callMethod( const QString &method, const Args &...args ) const
{
  if ( !isConnected() )
  {
    return std::nullopt;
  }

  QDBusReply< T > reply = m_interface->call( method, args... );
  if ( reply.isValid() )
  {
    return reply.value();
  }
  else
  {
    qWarning() << "DBus call failed:" << method << "-" << reply.error().message();
    return std::nullopt;
  }
}

bool TccdClient::callVoidMethod( const QString &method ) const
{
  if ( !isConnected() )
  {
    return false;
  }

  QDBusMessage reply = m_interface->call( method );
  if ( reply.type() == QDBusMessage::ErrorMessage )
  {
    qWarning() << "DBus call failed:" << method << "-" << reply.errorMessage();
    return false;
  }
  return true;
}

template< typename... Args >
bool TccdClient::callVoidMethod( const QString &method, const Args &...args ) const
{
  if ( !isConnected() )
  {
    return false;
  }

  QDBusMessage reply = m_interface->call( method, args... );
  if ( reply.type() == QDBusMessage::ErrorMessage )
  {
    qWarning() << "DBus call failed:" << method << "-" << reply.errorMessage();
    return false;
  }
  return true;
}

// Profile Management
std::optional< std::string > TccdClient::getDefaultProfilesJSON()
{
  if ( auto result = callMethod< QString >( "GetDefaultProfilesJSON" ) )
  {
    return result->toStdString();
  }
  return std::nullopt;
}

std::optional< std::string > TccdClient::getCustomProfilesJSON()
{
  if ( auto result = callMethod< QString >( "GetCustomProfilesJSON" ) )
  {
    return result->toStdString();
  }
  return std::nullopt;
}

std::optional< std::string > TccdClient::getActiveProfileJSON()
{
  if ( auto result = callMethod< QString >( "GetActiveProfileJSON" ) )
  {
    return result->toStdString();
  }
  return std::nullopt;
}

bool TccdClient::setActiveProfile( const std::string &profileId )
{
  return callVoidMethod( "SetActiveProfile", QString::fromStdString( profileId ) );
}

bool TccdClient::saveCustomProfile( const std::string &profileJSON )
{
  return callVoidMethod( "SaveCustomProfile", QString::fromStdString( profileJSON ) );
}

bool TccdClient::deleteCustomProfile( const std::string &profileId )
{
  return callVoidMethod( "DeleteCustomProfile", QString::fromStdString( profileId ) );
}

// Display Control
bool TccdClient::setDisplayBrightness( int brightness )
{
  return callVoidMethod( "SetDisplayBrightness", brightness );
}

std::optional< int > TccdClient::getDisplayBrightness()
{
  return callMethod< int >( "GetDisplayBrightness" );
}

// Webcam Control
bool TccdClient::setWebcamEnabled( bool enabled )
{
  return callVoidMethod( "SetWebcam", enabled );
}

std::optional< bool > TccdClient::getWebcamEnabled()
{
  return callMethod< bool >( "GetWebcam" );
}

// GPU Info
std::optional< std::string > TccdClient::getGpuInfo()
{
  if ( auto result = callMethod< QString >( "GetGpuInfo" ) )
  {
    return result->toStdString();
  }
  return std::nullopt;
}

// Fn Lock
bool TccdClient::setFnLock( bool enabled )
{
  return callVoidMethod( "SetFnLock", enabled );
}

std::optional< bool > TccdClient::getFnLock()
{
  return callMethod< bool >( "GetFnLock" );
}

// Stub implementations for remaining methods
// TODO: Implement these based on actual tccd-ng DBus interface

bool TccdClient::setYCbCr420Workaround( bool enabled )
{
  // TODO: Implement when DBus method is available
  return false;
}

std::optional< bool > TccdClient::getYCbCr420Workaround()
{
  return std::nullopt;
}

bool TccdClient::setDisplayRefreshRate( const std::string &display, int refreshRate )
{
  return false;
}

bool TccdClient::setCpuScalingGovernor( const std::string &governor )
{
  return false;
}

std::optional< std::string > TccdClient::getCpuScalingGovernor()
{
  return std::nullopt;
}

std::optional< std::vector< std::string > > TccdClient::getAvailableCpuGovernors()
{
  return std::nullopt;
}

bool TccdClient::setCpuFrequency( int minFreq, int maxFreq )
{
  return false;
}

bool TccdClient::setEnergyPerformancePreference( const std::string &preference )
{
  return false;
}

bool TccdClient::setFanProfile( const std::string &profileJSON )
{
  return false;
}

std::optional< std::string > TccdClient::getCurrentFanSpeed()
{
  return std::nullopt;
}

std::optional< std::string > TccdClient::getFanTemperatures()
{
  return std::nullopt;
}

bool TccdClient::setODMPowerLimits( const std::vector< int > &limits )
{
  return false;
}

std::optional< std::vector< int > > TccdClient::getODMPowerLimits()
{
  return std::nullopt;
}

bool TccdClient::setChargingProfile( int startThreshold, int endThreshold, int chargeType )
{
  return false;
}

std::optional< std::string > TccdClient::getChargingState()
{
  return std::nullopt;
}

bool TccdClient::setNVIDIAPowerOffset( int offset )
{
  return false;
}

std::optional< int > TccdClient::getNVIDIAPowerOffset()
{
  return std::nullopt;
}

bool TccdClient::setPrimeProfile( const std::string &profile )
{
  return false;
}

std::optional< std::string > TccdClient::getPrimeProfile()
{
  return std::nullopt;
}

bool TccdClient::setKeyboardBacklight( const std::string &config )
{
  return false;
}

std::optional< std::string > TccdClient::getKeyboardBacklightInfo()
{
  return std::nullopt;
}

bool TccdClient::setODMPerformanceProfile( const std::string &profile )
{
  return false;
}

std::optional< std::string > TccdClient::getODMPerformanceProfile()
{
  return std::nullopt;
}

std::optional< std::vector< std::string > > TccdClient::getAvailableODMProfiles()
{
  return std::nullopt;
}

void TccdClient::subscribeProfileChanged( ProfileChangedCallback callback )
{
  // Already handled via Qt signal connection
}

void TccdClient::subscribePowerStateChanged( PowerStateChangedCallback callback )
{
  // Already handled via Qt signal connection
}

} // namespace ucc
