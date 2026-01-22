// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Uniwill Control Center Contributors

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents

PlasmoidItem {
    id: root

    width: PlasmaCore.Units.gridUnit * 8
    height: PlasmaCore.Units.gridUnit * 4

    Plasmoid.backgroundHints: PlasmaCore.Types.DefaultBackground

    property string activeProfile: "Default"
    property var profiles: ["Default", "Cool and breezy", "Powersave extreme"]

    Timer {
        interval: 5000
        running: true
        repeat: true
        onTriggered: updateActiveProfile()
    }

    Component.onCompleted: {
        updateActiveProfile()
    }

    function updateActiveProfile() {
        // TODO: Connect to DBus and fetch active profile
    }

    function setProfile(profileId) {
        // TODO: Call DBus to set profile
        console.log("Setting profile:", profileId)
    }

    fullRepresentation: Item {
        Layout.preferredWidth: PlasmaCore.Units.gridUnit * 15
        Layout.preferredHeight: PlasmaCore.Units.gridUnit * 12

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: PlasmaCore.Units.smallSpacing
            spacing: PlasmaCore.Units.largeSpacing

            PlasmaComponents.Label {
                text: "Profile Switcher"
                font.pointSize: 14
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            PlasmaComponents.Label {
                text: "Active: " + root.activeProfile
                Layout.alignment: Qt.AlignHCenter
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: root.profiles
                spacing: PlasmaCore.Units.smallSpacing

                delegate: PlasmaComponents.Button {
                    width: ListView.view.width
                    text: modelData
                    highlighted: modelData === root.activeProfile
                    onClicked: root.setProfile(modelData)
                }
            }
        }
    }

    compactRepresentation: Item {
        PlasmaComponents.Label {
            anchors.centerIn: parent
            text: root.activeProfile
            elide: Text.ElideRight
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.expanded = !root.expanded
        }
    }
}
