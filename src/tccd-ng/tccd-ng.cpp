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

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <cstring>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Hardware abstraction layer
#include "tuxedo_io_lib/tuxedo_io_api.hh"
#include "workers/GpuInfoWorker.hpp"
#include "TccDBusService.hpp"
#include "SettingsManager.hpp"

// C++20 features used
constexpr std::string_view VERSION = "1.0.0-ng";
constexpr std::string_view DAEMON_NAME = "tccd-ng";

// Signal handling for graceful shutdown
volatile bool g_shutdown_requested = false;

void signal_handler( int signum )
{
  if ( signum == SIGTERM or signum == SIGINT )
  {
    g_shutdown_requested = true;
  }
}

// Daemonize process
void daemonize()
{
  // Fork the parent process
  pid_t pid = fork();
  if ( pid < 0 )
  {
    std::cerr << "Failed to fork process" << std::endl;
    exit( 1 );
  }

  // Exit parent process
  if ( pid > 0 )
  {
    exit( 0 );
  }

  // Create new session
  if ( setsid() < 0 )
  {
    std::cerr << "Failed to create new session" << std::endl;
    exit( 1 );
  }

  // Change working directory
  if ( chdir( "/" ) < 0 )
  {
    std::cerr << "Failed to change directory to /" << std::endl;
    exit( 1 );
  }

  // Redirect standard file descriptors
  int devnull = open( "/dev/null", O_RDWR );
  if ( devnull < 0 )
  {
    std::cerr << "Failed to open /dev/null" << std::endl;
    exit( 1 );
  }

  dup2( devnull, STDIN_FILENO );
  dup2( devnull, STDOUT_FILENO );
  dup2( devnull, STDERR_FILENO );

  if ( devnull > 2 )
  {
    close( devnull );
  }
}

// Initialize syslog
void init_syslog()
{
  openlog( DAEMON_NAME.data(), LOG_PID, LOG_DAEMON );
  syslog( LOG_INFO, "TuxedoControlCenterDaemon-NG (tccd-ng) starting - version %s", VERSION.data() );
}

// Cleanup syslog
void cleanup_syslog()
{
  syslog( LOG_INFO, "TuxedoControlCenterDaemon-NG (tccd-ng) shutting down" );
  closelog();
}

// Main daemon loop
int run_daemon()
{
  init_syslog();

  // Set up signal handlers
  signal( SIGTERM, signal_handler );
  signal( SIGINT, signal_handler );
  signal( SIGHUP, signal_handler );

  syslog( LOG_INFO, "Daemon initialized successfully" );

  // Initialize hardware interface
  try
  {
    TuxedoIOAPI io;
    syslog( LOG_INFO, "Hardware interface initialized" );

    // Detect device capabilities
    bool identified = false;
    if ( io.identify( identified ) and identified )
    {
      std::string interface_id, model_id;
      if ( io.deviceInterfaceIdStr( interface_id ) )
      {
        syslog( LOG_INFO, "Detected interface: %s", interface_id.c_str() );
      }
      if ( io.deviceModelIdStr( model_id ) )
      {
        syslog( LOG_INFO, "Detected model: %s", model_id.c_str() );
      }
    }
    else
    {
      syslog( LOG_WARNING, "No compatible hardware device detected" );
    }
  }
  catch ( const std::exception& e )
  {
    syslog( LOG_ERR, "Failed to initialize hardware interface: %s", e.what() );
  }

  // Main event loop
  while ( not g_shutdown_requested )
  {
    // TODO: Implement main daemon functionality
    // - Monitor hardware state
    // - Apply settings
    // - Respond to DBus requests
    // - Manage fan control, power profiles, etc.

    sleep( 1 );
  }

  cleanup_syslog();
  return 0;
}

// Print usage information
void print_usage( std::string_view program_name )
{
  std::cout << "Usage: " << program_name << " [OPTIONS]\n"
            << "Options:\n"
            << "  --version      Show version information\n"
            << "  --help         Show this help message\n"
            << "  --debug        Run in debug mode (foreground with verbose logging)\n"
            << "  --start        Start the daemon\n";
}

// Print version information
void print_version()
{
  std::cout << DAEMON_NAME << " version " << VERSION << "\n"
            << "Copyright (c) 2024-2026 TUXEDO Computers GmbH\n"
            << "Compiled with C++20 standard\n";
}

int main( int argc, char* argv[] )
{
  std::vector<std::string> arguments;
  for ( int i = 1; i < argc; ++i )
  {
    arguments.push_back( argv[ i ] );
  }

  bool debug_mode = false;
  bool start_daemon = false;
  std::string new_settings_path;

  // parse command-line arguments
  for ( size_t i = 0; i < arguments.size(); ++i )
  {
    const auto& arg = arguments[i];

    if ( arg == "--version" or arg == "-v" )
    {
      print_version();
      return 0;
    }
    else if ( arg == "--help" or arg == "-h" )
    {
      print_usage( argv[ 0 ] );
      return 0;
    }
    else if ( arg == "--debug" )
    {
      debug_mode = true;
    }
    else if ( arg == "--start" )
    {
      start_daemon = true;
    }
    else if ( arg == "--new_settings" )
    {
      if ( i + 1 < arguments.size() )
      {
        new_settings_path = arguments[i + 1];
        ++i; // skip next argument
      }
      else
      {
        std::cerr << "Error: --new_settings requires a file path argument" << std::endl;
        return 1;
      }
    }
  }

  // handle --new_settings: read settings from file and update /etc/tcc/settings
  if ( not new_settings_path.empty() )
  {
    try
    {
      std::ifstream file( new_settings_path );

      if ( not file )
      {
        std::cerr << "[Settings] Failed to open new settings file: " << new_settings_path << std::endl;
        return 1;
      }

      std::string content( ( std::istreambuf_iterator< char >( file ) ),
                           std::istreambuf_iterator< char >() );
      file.close();

      SettingsManager settingsManager;
      auto parsedSettings = settingsManager.parseSettingsJSON( content );

      if ( not parsedSettings.has_value() )
      {
        std::cerr << "[Settings] Failed to parse new settings file" << std::endl;
        return 1;
      }

      if ( settingsManager.writeSettings( *parsedSettings ) )
      {
        std::cout << "[Settings] Successfully updated settings from " << new_settings_path << std::endl;
        return 0;
      }
      else
      {
        std::cerr << "[Settings] Failed to write updated settings" << std::endl;
        return 1;
      }
    }
    catch ( const std::exception &e )
    {
      std::cerr << "[Settings] Exception handling new_settings: " << e.what() << std::endl;
      return 1;
    }
  }

  // default action is to start
  if ( not debug_mode and not start_daemon and arguments.empty() )
  {
    start_daemon = true;
  }

  TccDBusService dbusService;

  while ( true )
  {

    sleep( 1 );
  }
#if 0
  if ( debug_mode )
  {
    // Debug mode: run in foreground with logging to console
    std::cout << DAEMON_NAME << " version " << VERSION << " - Debug mode" << std::endl;
    std::cout << "Running in foreground with console logging..." << std::endl;
    // TODO: Implement debug logging
    return run_daemon();
  }
  else if ( start_daemon )
  {
    // Normal mode: daemonize and run
    daemonize();
    return run_daemon();
  }
#endif
  return 0;
}
