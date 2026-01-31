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

#include "DetectLayout.hpp"
#include <QProcess>
#include <QDebug>
#include <QCoreApplication>

namespace ucc
{

QString detectKeyboardLayout()
{
  QProcess process;
  
  // Detect desktop environment
  QString desktop = qgetenv( "XDG_CURRENT_DESKTOP" ).toLower();
  QString session = qgetenv( "DESKTOP_SESSION" ).toLower();
  bool isKDE = desktop.contains( "kde" ) || session.contains( "kde" ) || !qgetenv( "KDE_SESSION_UID" ).isEmpty();
  bool isGNOME = desktop.contains( "gnome" ) || session.contains( "gnome" );
  bool isWayland = !qgetenv( "WAYLAND_DISPLAY" ).isEmpty();
  
  qDebug() << "Detected DE - KDE:" << isKDE << "GNOME:" << isGNOME << "Wayland:" << isWayland;
  
  // Try desktop environment specific detection first
  if ( isKDE )
  {
    qDebug() << "Trying KDE detection";
    
    // Try DBus for current KDE keyboard layout
    qDebug() << "Trying DBus for KDE layout";
    // Get current index
    process.start("dbus-send", QStringList() << "--session" << "--dest=org.kde.keyboard" << "--print-reply" << "/Layouts" << "org.kde.KeyboardLayouts.getLayout");
    if (process.waitForFinished(3000)) {
      QString output = process.readAllStandardOutput().trimmed();
      qDebug() << "dbus-send getLayout output:" << output;
      // Parse the output, it should be uint32 <number>
      QStringList lines = output.split('\n');
      uint currentIndex = 0;
      for (const QString &line : lines) {
        if (line.contains("uint32")) {
          currentIndex = line.split(' ').last().toUInt();
          break;
        }
      }
      qDebug() << "currentIndex:" << currentIndex;
      
      // Get layouts list
      process.start("dbus-send", QStringList() << "--session" << "--dest=org.kde.keyboard" << "--print-reply" << "/Layouts" << "org.kde.KeyboardLayouts.getLayoutsList");
      if (process.waitForFinished(3000)) {
        QString listOutput = process.readAllStandardOutput().trimmed();
        qDebug() << "dbus-send getLayoutsList output:" << listOutput;
        // Parse the array
        QStringList listLines = listOutput.split('\n');
        QStringList layouts;
        int count = 0;
        for (const QString &line : listLines) {
          if (line.contains("string \"")) {
            QString value = line.split('"').at(1);
            if (count % 3 == 0) {
              layouts.append(value);
            }
            count++;
          }
        }
        qDebug() << "layouts:" << layouts;
        if (currentIndex < layouts.size()) {
          QString layout = layouts.at(currentIndex);
          qDebug() << "Detected KDE layout:" << layout;
          return layout;
        }
      }
    }
  }
  else if ( isGNOME )
  {
    qDebug() << "Trying GNOME detection";
    
    // Try gsettings for GNOME
    process.start( "gsettings", QStringList() << "get" << "org.gnome.desktop.input-sources" << "sources" );
    if ( process.waitForFinished( 3000 ) )
    {
      QString output = process.readAllStandardOutput();
      qDebug() << "gsettings output:" << output;
      if ( output.contains( "de" ) )
      {
        qDebug() << "Detected German from GNOME gsettings";
        return "de";
      }
    }
  }
  
  // Try generic/system-wide detection
  qDebug() << "Trying generic detection";
  
  // Try localectl (systemd - works on most Linux systems)
  process.start( "localectl" );
  if ( process.waitForFinished( 3000 ) )
  {
    QString output = process.readAllStandardOutput();
    qDebug() << "localectl output:" << output;
    
    QStringList lines = output.split( '\n' );
    for ( const QString &line : lines )
    {
      if ( line.contains( "X11 Layout:" ) || line.contains( "VC Keymap:" ) )
      {
        QString layout = line.split( ':' ).value( 1 ).trimmed();
        qDebug() << "Detected layout from localectl:" << layout;
        if ( layout.contains( "de", Qt::CaseInsensitive ) )
          return "de";
        if ( !layout.isEmpty() && layout != "n/a" )
          return layout.split( ',' ).first();
      }
    }
  }
  
  // Try setxkbmap (works on X11, may give wrong results on Wayland)
  process.start( "setxkbmap", QStringList() << "-query" );
  if ( process.waitForFinished( 3000 ) )
  {
    QString output = process.readAllStandardOutput();
    qDebug() << "setxkbmap output:" << output;
    
    QStringList lines = output.split( '\n' );
    for ( const QString &line : lines )
    {
      if ( line.startsWith( "layout:" ) )
      {
        QString layout = line.split( ':' ).value( 1 ).trimmed();
        qDebug() << "Detected layout from setxkbmap:" << layout;
        if ( layout.contains( "de", Qt::CaseInsensitive ) )
          return "de";
        // On Wayland, setxkbmap might be unreliable, so don't return "us" immediately
        if ( !isWayland && !layout.isEmpty() )
          return layout;
        if ( layout != "us" && !layout.isEmpty() )
          return layout;
      }
    }
  }
  
  // Try compositor-specific detection on Wayland
  if ( isWayland )
  {
    qDebug() << "Trying Wayland compositor detection";
    
    // Try swaymsg for Sway
    process.start( "swaymsg", QStringList() << "-t" << "get_inputs" );
    if ( process.waitForFinished( 3000 ) )
    {
      QString output = process.readAllStandardOutput();
      qDebug() << "swaymsg output:" << output;
      if ( output.contains( "\"de\"" ) || output.contains( "'de'" ) )
      {
        qDebug() << "Detected German from swaymsg";
        return "de";
      }
    }
  }
  
  // Fallback: check environment variables
  QString xkbLayout = qgetenv( "XKB_DEFAULT_LAYOUT" );
  if ( !xkbLayout.isEmpty() )
  {
    qDebug() << "Detected layout from XKB_DEFAULT_LAYOUT:" << xkbLayout;
    if ( xkbLayout.contains( "de", Qt::CaseInsensitive ) )
      return "de";
    return xkbLayout.split( ',' ).first();
  }
  
  // Another fallback: check LANG or LC_CTYPE
  QString lang = qgetenv( "LANG" );
  if ( lang.contains( "de", Qt::CaseInsensitive ) )
  {
    qDebug() << "Detected German from LANG:" << lang;
    return "de";
  }
  
  qDebug() << "Defaulting to us layout";
  return "us"; // Default to US
}

}