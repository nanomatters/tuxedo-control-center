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

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ProfileManager.hpp"
#include "SystemMonitor.hpp"

int main( int argc, char *argv[] )
{
  QGuiApplication app( argc, argv );
  app.setOrganizationName( "UniwillControlCenter" );
  app.setOrganizationDomain( "uniwill.local" );
  app.setApplicationName( "ucc-gui" );
  app.setApplicationVersion( "0.1.0" );

  QQmlApplicationEngine engine;

  // Register C++ types with QML
  qmlRegisterType< ucc::ProfileManager >( "UCC", 1, 0, "ProfileManager" );
  qmlRegisterType< ucc::SystemMonitor >( "UCC", 1, 0, "SystemMonitor" );

  // Create root context objects
  auto profileManager = new ucc::ProfileManager( &app );
  auto systemMonitor = new ucc::SystemMonitor( &app );

  engine.rootContext()->setContextProperty( "profileManager", profileManager );
  engine.rootContext()->setContextProperty( "systemMonitor", systemMonitor );

  const QUrl url( QStringLiteral( "qrc:/qml/main.qml" ) );
  QObject::connect( &engine, &QQmlApplicationEngine::objectCreated,
                    &app, [url]( QObject *obj, const QUrl &objUrl ) {
    if ( !obj && url == objUrl )
      QCoreApplication::exit( -1 );
  }, Qt::QueuedConnection );

  engine.load( url );

  return app.exec();
}
