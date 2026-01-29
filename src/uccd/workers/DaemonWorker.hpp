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

#include <chrono>
#include <thread>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

/**
 * @brief Abstract base class for daemon worker threads
 *
 * Provides a framework for periodic work execution with automatic timer management.
 * Subclasses must implement onStart(), onWork(), and onExit() lifecycle methods.
 *
 * The worker automatically creates a background thread that:
 *   - Calls onStart() once at initialization
 *   - Calls onWork() repeatedly at the specified timeout interval
 *   - Calls onExit() during cleanup
 *
 * Uses C++20 jthread for automatic cleanup and stop token support.
 *
 * Usage:
 *   - Create concrete subclass inheriting from DaemonWorker
 *   - Implement pure virtual methods: onStart(), onWork(), onExit()
 *   - Constructor automatically starts the timer thread
 *   - Call stop() to gracefully shutdown the worker
 */
class DaemonWorker
{
public:
  /**
   * @brief Constructor
   *
   * Creates the worker and automatically starts the timer thread.
   * The thread will:
   *   1. Call onStart() once on startup
   *   2. Repeatedly call onWork() at timeout intervals
   *   3. Call onExit() when the thread stops
   *
   * @param timeout Duration in milliseconds between work cycles
   */
  explicit DaemonWorker( std::chrono::milliseconds timeout, bool autoStart = true ) noexcept
    : m_timeout( timeout ), m_isRunning( false )
  {
    if ( autoStart )
      start();
  }

  /**
   * @brief Virtual destructor
   *
   * Automatically stops the timer thread and calls onExit().
   * The jthread will be joined automatically.
   */
  virtual ~DaemonWorker() noexcept
  {
    stop();
  }

  // Prevent copy and move operations for this base class
  DaemonWorker( const DaemonWorker & ) = delete;
  DaemonWorker( DaemonWorker && ) = delete;
  DaemonWorker &operator=( const DaemonWorker & ) = delete;
  DaemonWorker &operator=( DaemonWorker && ) = delete;

  /**
   * @brief Gracefully stop the worker thread
   *
   * Signals the timer thread to stop, waits for onExit() to complete,
   * and joins the thread. The jthread will be automatically joined
   * on destruction.
   */
  void stop() noexcept
  {
    if ( m_isRunning )
    {
      m_isRunning = false;
      {
        std::unique_lock< std::mutex > lock( m_stateMutex );
        m_stateCV.wait_for( lock, std::chrono::milliseconds( 100 ),
                            [ this ]() { return not m_isRunning; } );
      }
    }
  }

  /**
   * @brief Start the worker thread
   */
  void start() noexcept
  {
    if ( m_isRunning )
      return;

    m_isRunning = true;
    m_timer = std::jthread( &DaemonWorker::timerLoop, this );
  }

  /**
   * @brief Get the timeout duration
   * @return The timeout in milliseconds
   */
  [[nodiscard]] std::chrono::milliseconds getTimeout() const noexcept
  {
    return m_timeout;
  }

  /**
   * @brief Check if the worker is running
   * @return True if the worker thread is active
   */
  [[nodiscard]] bool isRunning() const noexcept
  {
    return m_isRunning;
  }

protected:
  /**
   * @brief Called once when the worker starts
   *
   * Invoked by the timer thread on startup, before the periodic work loop begins.
   * Must be implemented by derived classes.
   */
  virtual void onStart() = 0;

  /**
   * @brief Called repeatedly during the work cycle
   *
   * Invoked by the timer thread at each timeout interval.
   * Must be implemented by derived classes.
   */
  virtual void onWork() = 0;

  /**
   * @brief Called when the worker exits
   *
   * Invoked by the timer thread when stop() is called.
   * Must be implemented by derived classes.
   */
  virtual void onExit() = 0;

private:
  /**
   * @brief Main timer loop running in the background thread
   *
   * This function:
   *   1. Calls onStart() once at the beginning
   *   2. Loops calling onWork() at timeout intervals
   *   3. Calls onExit() before exiting
   *   4. Stops when m_isRunning becomes false
   *
   * @param stopToken Token for checking if the thread should stop (jthread)
   */
  void timerLoop( std::stop_token stopToken ) noexcept
  {
    try
    {
      onStart();

      while ( m_isRunning and not stopToken.stop_requested() )
      {
        onWork();
        std::this_thread::sleep_for( m_timeout );
      }

      onExit();
    }
    catch ( const std::exception & )
    {
      // silently ignore exceptions to prevent thread from crashing
    }

    {
      std::unique_lock< std::mutex > lock( m_stateMutex );
      m_isRunning = false;
      m_stateCV.notify_all();
    }
  }

  const std::chrono::milliseconds m_timeout;
  std::jthread m_timer;
  std::atomic< bool > m_isRunning;
  std::mutex m_stateMutex;
  std::condition_variable m_stateCV;
};
