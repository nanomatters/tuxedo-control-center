// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Uniwill Control Center Contributors

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents

PlasmoidItem {
    id: root

    width: PlasmaCore.Units.gridUnit * 10
    height: PlasmaCore.Units.gridUnit * 10

    Plasmoid.backgroundHints: PlasmaCore.Types.DefaultBackground

    property string cpuTemp: "0°C"
    property string gpuTemp: "0°C"
    property int cpuUsage: 0
    property int fanSpeed: 0

    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: updateMetrics()
    }

    Component.onCompleted: {
        updateMetrics()
    }

    function updateMetrics() {
        // TODO: Connect to DBus and fetch real metrics
        // This is a placeholder implementation
        cpuTemp = Math.floor(Math.random() * 30 + 40) + "°C"
        gpuTemp = Math.floor(Math.random() * 30 + 50) + "°C"
        cpuUsage = Math.floor(Math.random() * 100)
        fanSpeed = Math.floor(Math.random() * 3000 + 1000)
    }

    fullRepresentation: Item {
        Layout.preferredWidth: PlasmaCore.Units.gridUnit * 15
        Layout.preferredHeight: PlasmaCore.Units.gridUnit * 20

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: PlasmaCore.Units.smallSpacing
            spacing: PlasmaCore.Units.largeSpacing

            PlasmaComponents.Label {
                text: "System Monitor"
                font.pointSize: 14
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            GridLayout {
                columns: 2
                rowSpacing: PlasmaCore.Units.smallSpacing
                columnSpacing: PlasmaCore.Units.largeSpacing
                Layout.fillWidth: true

                PlasmaComponents.Label {
                    text: "CPU Temperature:"
                    font.bold: true
                }
                PlasmaComponents.Label {
                    text: root.cpuTemp
                }

                PlasmaComponents.Label {
                    text: "GPU Temperature:"
                    font.bold: true
                }
                PlasmaComponents.Label {
                    text: root.gpuTemp
                }

                PlasmaComponents.Label {
                    text: "CPU Usage:"
                    font.bold: true
                }
                PlasmaComponents.Label {
                    text: root.cpuUsage + "%"
                }

                PlasmaComponents.Label {
                    text: "Fan Speed:"
                    font.bold: true
                }
                PlasmaComponents.Label {
                    text: root.fanSpeed + " RPM"
                }
            }

            Item {
                Layout.fillHeight: true
            }

            PlasmaComponents.Button {
                text: "Open Control Center"
                Layout.fillWidth: true
                onClicked: {
                    executable = "ucc-gui"
                    // TODO: Launch ucc-gui
                }
            }
        }
    }

    compactRepresentation: Item {
        PlasmaCore.IconItem {
            anchors.fill: parent
            source: "preferences-system"

            PlasmaComponents.Label {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                text: root.cpuTemp
                font.pixelSize: 10
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.expanded = !root.expanded
        }
    }
}
