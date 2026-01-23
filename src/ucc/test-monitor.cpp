/*
 * Quick test for SystemMonitor DBus connectivity
 */

#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include "../libucc-dbus/TccdClient.hpp"

using namespace std;

int main( int argc, char *argv[] )
{
  QCoreApplication app( argc, argv );

  ucc::TccdClient client;

  cout << "=== UCC SystemMonitor Test ===" << endl;
  cout << "Testing DBus connection to tccd-ng daemon..." << endl;
  cout << "Connected: " << (client.isConnected() ? "YES" : "NO") << endl;

  if ( !client.isConnected() )
  {
    cout << "ERROR: Cannot connect to tccd-ng daemon" << endl;
    cout << "Make sure tccd-ng is running on system bus" << endl;
    return 1;
  }

  cout << "\n=== Testing monitoring methods ===" << endl;

  // Test CPU Temperature
  if ( auto temp = client.getCpuTemperature() )
  {
    cout << "✓ CPU Temperature: " << *temp << "°C" << endl;
  }
  else
  {
    cout << "✗ CPU Temperature: FAILED" << endl;
  }

  // Test CPU Frequency
  if ( auto freq = client.getCpuFrequency() )
  {
    cout << "✓ CPU Frequency: " << *freq << " MHz" << endl;
  }
  else
  {
    cout << "✗ CPU Frequency: FAILED" << endl;
  }

  // Test CPU Power
  if ( auto power = client.getCpuPower() )
  {
    cout << "✓ CPU Power: " << *power << " W" << endl;
  }
  else
  {
    cout << "✗ CPU Power: FAILED" << endl;
  }

  // Test GPU Temperature
  if ( auto temp = client.getGpuTemperature() )
  {
    cout << "✓ GPU Temperature: " << *temp << "°C" << endl;
  }
  else
  {
    cout << "✗ GPU Temperature: FAILED" << endl;
  }

  // Test GPU Frequency
  if ( auto freq = client.getGpuFrequency() )
  {
    cout << "✓ GPU Frequency: " << *freq << " MHz" << endl;
  }
  else
  {
    cout << "✗ GPU Frequency: FAILED" << endl;
  }

  // Test GPU Power
  if ( auto power = client.getGpuPower() )
  {
    cout << "✓ GPU Power: " << *power << " W" << endl;
  }
  else
  {
    cout << "✗ GPU Power: FAILED" << endl;
  }

  // Test Fan Speed
  if ( auto rpm = client.getFanSpeedRPM() )
  {
    cout << "✓ Fan Speed: " << *rpm << " RPM" << endl;
  }
  else
  {
    cout << "✗ Fan Speed: FAILED" << endl;
  }

  // Test GPU Fan Speed
  if ( auto rpm = client.getGpuFanSpeedRPM() )
  {
    cout << "✓ GPU Fan Speed: " << *rpm << " RPM" << endl;
  }
  else
  {
    cout << "✗ GPU Fan Speed: FAILED" << endl;
  }

  cout << "\n=== Test complete ===" << endl;

  return 0;
}
