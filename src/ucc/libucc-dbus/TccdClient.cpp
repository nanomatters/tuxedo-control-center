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
#include <QDBusArgument>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QThread>
#include <QFile>
#include <QVariantMap>
#include <QRegularExpression>
#include <QSet>

namespace ucc
{

namespace
{
QSet< QString > loadSupportedMethods( QDBusInterface *interface, bool &ok )
{
  ok = false;
  QSet< QString > methods;
  if ( !interface || !interface->isValid() )
  {
    return methods;
  }

  QDBusInterface introspectIface(
    interface->service(),
    interface->path(),
    "org.freedesktop.DBus.Introspectable",
    interface->connection() );

  QDBusMessage reply = introspectIface.call( "Introspect" );
  if ( reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty() )
  {
    return methods;
  }

  const QString xml = reply.arguments().at( 0 ).toString();
  QRegularExpression re( R"(<method name=\"([^\"]+)\")" );
  auto it = re.globalMatch( xml );
  while ( it.hasNext() )
  {
    methods.insert( it.next().captured( 1 ) );
  }

  ok = true;
  return methods;
}

bool hasMethod( QDBusInterface *interface, const QString &method )
{
  static bool initialized = false;
  static bool introspectOk = false;
  static QSet< QString > cached;

  if ( !initialized )
  {
    cached = loadSupportedMethods( interface, introspectOk );
    initialized = true;
  }

  if ( !introspectOk )
  {
    return false;
  }

  return cached.contains( method );
}
} // namespace

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

std::optional< std::string > TccdClient::getDefaultValuesProfileJSON()
{
  if ( auto result = callMethod< QString >( "GetDefaultValuesProfileJSON" ) )
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

std::optional< std::string > TccdClient::getSettingsJSON()
{
  if ( auto result = callMethod< QString >( "GetSettingsJSON" ) )
  {
    return result->toStdString();
  }
  return std::nullopt;
}

std::optional< std::string > TccdClient::getPowerState()
{
  if ( auto result = callMethod< QString >( "GetPowerState" ) )
  {
    return result->toStdString();
  }
  return std::nullopt;
}

bool TccdClient::setStateMap( const std::string &state, const std::string &profileId )
{
  const QString qState = QString::fromStdString( state );
  const QString qProfileId = QString::fromStdString( profileId );
  return callVoidMethod( "SetStateMap", qState, qProfileId );
}

bool TccdClient::setActiveProfile( const std::string &profileId )
{
  const QString id = QString::fromStdString( profileId );
  if ( hasMethod( m_interface.get(), "SetActiveProfile" ) )
  {
    return callVoidMethod( "SetActiveProfile", id );
  }
  if ( hasMethod( m_interface.get(), "SetTempProfileById" ) )
  {
    return callVoidMethod( "SetTempProfileById", id );
  }

  return false;
}

bool TccdClient::applyProfile( const std::string &profileJSON )
{
  return callVoidMethod( "ApplyProfile", QString::fromStdString( profileJSON ) );
}

bool TccdClient::saveCustomProfile( [[maybe_unused]] [[maybe_unused]] const std::string &profileJSON )
{
  return callVoidMethod( "SaveCustomProfile", QString::fromStdString( profileJSON ) );
}

bool TccdClient::deleteCustomProfile( [[maybe_unused]] const std::string &profileId )
{
  return callVoidMethod( "DeleteCustomProfile", QString::fromStdString( profileId ) );
}

std::optional< std::string > TccdClient::getFanProfile( const std::string &name )
{
  if ( auto result = callMethod< QString >( "GetFanProfile", QString::fromStdString( name ) ) )
  {
    return result->toStdString();
  }
  return std::nullopt;
}

std::optional< std::vector< std::string > > TccdClient::getFanProfileNames()
{
  if ( auto result = callMethod< QString >( "GetFanProfileNames" ) )
  {
    // Parse JSON array
    QJsonDocument doc = QJsonDocument::fromJson( result->toUtf8() );
    if ( doc.isArray() )
    {
      std::vector< std::string > names;
      for ( const auto &value : doc.array() )
      {
        if ( value.isString() )
        {
          names.push_back( value.toString().toStdString() );
        }
      }
      return names;
    }
  }
  return std::nullopt;
}

std::optional< bool > TccdClient::setFanProfile( const std::string &name, const std::string &json )
{
  if ( auto result = callMethod< bool >( "SetFanProfile", QString::fromStdString( name ), QString::fromStdString( json ) ) )
  {
    return result.value();
  }
  return std::nullopt;
}

// Display Control
bool TccdClient::setDisplayBrightness( int brightness )
{
  if ( hasMethod( m_interface.get(), "SetDisplayBrightness" ) )
  {
    return callVoidMethod( "SetDisplayBrightness", brightness );
  }

  return false;
}

std::optional< int > TccdClient::getDisplayBrightness()
{
  if ( hasMethod( m_interface.get(), "GetDisplayBrightness" ) )
  {
    return callMethod< int >( "GetDisplayBrightness" );
  }

  return std::nullopt;
}

// Webcam Control
bool TccdClient::setWebcamEnabled( bool enabled )
{
  if ( hasMethod( m_interface.get(), "SetWebcam" ) )
  {
    return callVoidMethod( "SetWebcam", enabled );
  }

  return false;
}

std::optional< bool > TccdClient::getWebcamEnabled()
{
  if ( hasMethod( m_interface.get(), "GetWebcamSWStatus" ) )
  {
    return callMethod< bool >( "GetWebcamSWStatus" );
  }
  if ( hasMethod( m_interface.get(), "GetWebcam" ) )
  {
    return callMethod< bool >( "GetWebcam" );
  }

  return std::nullopt;
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
  if ( hasMethod( m_interface.get(), "SetFnLockStatus" ) )
  {
    return callVoidMethod( "SetFnLockStatus", enabled );
  }
  if ( hasMethod( m_interface.get(), "SetFnLock" ) )
  {
    return callVoidMethod( "SetFnLock", enabled );
  }

  return false;
}

std::optional< bool > TccdClient::getFnLock()
{
  if ( hasMethod( m_interface.get(), "GetFnLockStatus" ) )
  {
    return callMethod< bool >( "GetFnLockStatus" );
  }
  if ( hasMethod( m_interface.get(), "GetFnLock" ) )
  {
    return callMethod< bool >( "GetFnLock" );
  }

  return std::nullopt;
}

// Stub implementations for remaining methods
// TODO: Implement these based on actual tccd-ng DBus interface

bool TccdClient::setYCbCr420Workaround( [[maybe_unused]] bool enabled )
{
  // TODO: Implement when DBus method is available
  return false;
}

std::optional< bool > TccdClient::getYCbCr420Workaround()
{
  return std::nullopt;
}

bool TccdClient::setDisplayRefreshRate( [[maybe_unused]] const std::string &display, [[maybe_unused]] int refreshRate )
{
  return false;
}

bool TccdClient::setCpuScalingGovernor( [[maybe_unused]] const std::string &governor )
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

bool TccdClient::setCpuFrequency( [[maybe_unused]] int minFreq, [[maybe_unused]] int maxFreq )
{
  return false;
}

bool TccdClient::setEnergyPerformancePreference( [[maybe_unused]] const std::string &preference )
{
  return false;
}

bool TccdClient::setFanProfile( [[maybe_unused]] [[maybe_unused]] const std::string &profileJSON )
{
  return false;
}

bool TccdClient::setFanProfileCPU( const std::string &pointsJSON )
{
  const QString js = QString::fromStdString( pointsJSON );
  if ( hasMethod( m_interface.get(), "SetFanProfileCPU" ) )
  {
    return callMethod< bool, QString >( "SetFanProfileCPU", js ).value_or( false );
  }
  return false;
}

bool TccdClient::setFanProfileDGPU( const std::string &pointsJSON )
{
  const QString js = QString::fromStdString( pointsJSON );
  if ( hasMethod( m_interface.get(), "SetFanProfileDGPU" ) )
  {
    return callMethod< bool, QString >( "SetFanProfileDGPU", js ).value_or( false );
  }
  return false;
}

bool TccdClient::applyFanProfiles( const std::string &fanProfilesJSON )
{
  const QString js = QString::fromStdString( fanProfilesJSON );
  if ( hasMethod( m_interface.get(), "ApplyFanProfiles" ) )
  {
    return callMethod< bool, QString >( "ApplyFanProfiles", js ).value_or( false );
  }
  return false;
}

bool TccdClient::revertFanProfiles()
{
  if ( hasMethod( m_interface.get(), "RevertFanProfiles" ) )
  {
    return callMethod< bool >( "RevertFanProfiles" ).value_or( false );
  }
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

bool TccdClient::setODMPowerLimits( [[maybe_unused]] const std::vector< int > &limits )
{
  return false;
}

std::optional< std::vector< int > > TccdClient::getODMPowerLimits()
{
  return std::nullopt;
}

bool TccdClient::setChargingProfile( [[maybe_unused]] int startThreshold, [[maybe_unused]] int endThreshold, [[maybe_unused]] int chargeType )
{
  return false;
}

std::optional< std::string > TccdClient::getChargingState()
{
  return std::nullopt;
}

bool TccdClient::setNVIDIAPowerOffset( [[maybe_unused]] int offset )
{
  return false;
}

std::optional< int > TccdClient::getNVIDIAPowerOffset()
{
  return std::nullopt;
}

bool TccdClient::setPrimeProfile( [[maybe_unused]] const std::string &profile )
{
  return false;
}

std::optional< std::string > TccdClient::getPrimeProfile()
{
  return std::nullopt;
}

bool TccdClient::setKeyboardBacklight( [[maybe_unused]] const std::string &config )
{
  return false;
}

std::optional< std::string > TccdClient::getKeyboardBacklightInfo()
{
  return std::nullopt;
}

bool TccdClient::setODMPerformanceProfile( [[maybe_unused]] const std::string &profile )
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

namespace
{
std::optional< int > readFanDataValue( QDBusInterface *iface, const QString &method, const QString &key )
{
  if ( !iface )
  {
    return std::nullopt;
  }

  QDBusMessage reply = iface->call( method );
  if ( reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty() )
  {
    return std::nullopt;
  }

  const QDBusArgument arg = reply.arguments().at( 0 ).value< QDBusArgument >();
  arg.beginMap();
  while ( !arg.atEnd() )
  {
    arg.beginMapEntry();
    QString entryKey;
    QVariantMap innerMap;
    arg >> entryKey;
    arg >> innerMap;
    arg.endMapEntry();

    if ( entryKey == key )
    {
      if ( innerMap.contains( "data" ) )
      {
        int value = innerMap.value( "data" ).toInt();
        if ( value >= 0 )
        {
          return value;
        }
      }
      return std::nullopt;
    }
  }
  arg.endMap();

  return std::nullopt;
}

std::optional< int > readJsonInt( QDBusInterface *iface, const QString &method, const QString &key )
{
  if ( !iface )
  {
    return std::nullopt;
  }

  QDBusMessage reply = iface->call( method );
  if ( reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty() )
  {
    return std::nullopt;
  }

  const QString json = reply.arguments().at( 0 ).toString();
  QJsonDocument doc = QJsonDocument::fromJson( json.toUtf8() );
  if ( doc.isNull() || !doc.isObject() )
  {
    return std::nullopt;
  }

  QJsonObject obj = doc.object();
  if ( !obj.contains( key ) )
  {
    return std::nullopt;
  }

  int val = obj[ key ].toInt();
  return ( val >= 0 ) ? std::optional< int >( val ) : std::nullopt;
}

std::optional< double > readJsonDouble( QDBusInterface *iface, const QString &method, const QString &key )
{
  if ( !iface )
  {
    return std::nullopt;
  }

  QDBusMessage reply = iface->call( method );
  if ( reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty() )
  {
    return std::nullopt;
  }

  const QString json = reply.arguments().at( 0 ).toString();
  QJsonDocument doc = QJsonDocument::fromJson( json.toUtf8() );
  if ( doc.isNull() || !doc.isObject() )
  {
    return std::nullopt;
  }

  QJsonObject obj = doc.object();
  if ( !obj.contains( key ) )
  {
    return std::nullopt;
  }

  double val = obj[ key ].toDouble();
  return ( val >= 0.0 ) ? std::optional< double >( val ) : std::nullopt;
}
} // namespace

// System Monitoring implementations
std::optional< int > TccdClient::getCpuTemperature()
{
  return readFanDataValue( m_interface.get(), "GetFanDataCPU", "temp" );
}

std::optional< int > TccdClient::getGpuTemperature()
{
  if ( auto temp = readJsonInt( m_interface.get(), "GetDGpuInfoValuesJSON", "temp" ) )
  {
    return temp;
  }

  return readJsonInt( m_interface.get(), "GetIGpuInfoValuesJSON", "temp" );
}

std::optional< int > TccdClient::getCpuFrequency()
{
  QFile file( "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq" );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    QString content = QString::fromLatin1( file.readAll() ).trimmed();
    bool ok;
    int freq = content.toInt( &ok );
    if ( ok )
    {
      return freq / 1000;
    }
  }
  return std::nullopt;
}

std::optional< int > TccdClient::getGpuFrequency()
{
  if ( auto freq = readJsonInt( m_interface.get(), "GetDGpuInfoValuesJSON", "coreFrequency" ) )
  {
    return freq;
  }
  return readJsonInt( m_interface.get(), "GetDGpuInfoValuesJSON", "coreFreq" );
}

std::optional< double > TccdClient::getCpuPower()
{
  return readJsonDouble( m_interface.get(), "GetCpuPowerValuesJSON", "powerDraw" );
}

std::optional< double > TccdClient::getGpuPower()
{
  if ( auto power = readJsonDouble( m_interface.get(), "GetDGpuInfoValuesJSON", "powerDraw" ) )
  {
    return power;
  }
  return readJsonDouble( m_interface.get(), "GetIGpuInfoValuesJSON", "powerDraw" );
}

std::optional< int > TccdClient::getFanSpeedRPM()
{
  if ( auto percentage = readFanDataValue( m_interface.get(), "GetFanDataCPU", "speed" ) )
  {
    return ( *percentage ) * 60;
  }
  return std::nullopt;
}

std::optional< int > TccdClient::getGpuFanSpeedRPM()
{
  auto gpu1 = readFanDataValue( m_interface.get(), "GetFanDataGPU1", "speed" );
  auto gpu2 = readFanDataValue( m_interface.get(), "GetFanDataGPU2", "speed" );

  if ( gpu1 && gpu2 )
  {
    return static_cast< int >( ( *gpu1 + *gpu2 ) / 2 ) * 60;
  }
  if ( gpu1 )
  {
    return ( *gpu1 ) * 60;
  }
  if ( gpu2 )
  {
    return ( *gpu2 ) * 60;
  }
  return std::nullopt;
}

void TccdClient::subscribeProfileChanged( [[maybe_unused]] ProfileChangedCallback callback )
{
  // Already handled via Qt signal connection
}

void TccdClient::subscribePowerStateChanged( [[maybe_unused]] PowerStateChangedCallback callback )
{
  // Already handled via Qt signal connection
}

} // namespace ucc
